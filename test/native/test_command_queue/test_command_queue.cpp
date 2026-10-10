// FROZEN TESTS (Contract V1.3) – integration test of the REAL src/app/command_queue.cpp
// on the host. FreeRTOS is replaced by test/native/freertos_shim (threads = tasks,
// 1 tick = 1 ms). Covers CONTRACT §1 (reply assignment, serialisation) and §6.7
// (priority queue, overflow latch). Do not modify.
#define ADD_HOST_SHIM 1
#include <unity.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include <vector>
#include "app/command_queue.cpp"   // implementation under test (includes the shim headers)

// ---- simulated control task --------------------------------------------------------
static std::atomic<bool> g_run{false};
static std::atomic<bool> g_allow_normal{true};
static std::thread       g_consumer;

// Deterministic result per command so that every caller knows its own expected reply.
static ApiError expectedFor(double num) { return (ApiError)(-(1 + ((int)num % 9))); }

static void consumerLoop() {
    while (g_run) {
        Command c;
        if (cmdQueueReceive(c, g_allow_normal)) {
            cmdQueueReply(c, c.id == CmdId::SET ? expectedFor(c.num) : ApiError::OK);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500));   // ~1 kHz cycle
    }
}
static void startConsumer() { g_run = true; g_consumer = std::thread(consumerLoop); }
static void stopConsumer() { g_run = false; if (g_consumer.joinable()) g_consumer.join(); }

static void drain() { Command c; while (cmdQueueReceive(c, true)) {} }

void setUp() { cmdQueueInit(); drain(); g_allow_normal = true; }
void tearDown() { stopConsumer(); drain(); }

static Command mkCmd(CmdId id, double num = 0, CmdSource src = CmdSource::API) {
    Command c; c.id = id; c.num = num; c.source = src; return c;
}

// ---- tests ---------------------------------------------------------------------------
void test_concurrent_callers_get_their_own_reply() {
    startConsumer();
    const int T = 8, N = 60;
    std::atomic<int> ok{0}, busy{0}, wrong{0};
    std::vector<std::thread> th;
    for (int t = 0; t < T; ++t) {
        th.emplace_back([&, t] {
            for (int i = 0; i < N; ++i) {
                double num = t * 1000 + i;
                Command c = mkCmd(CmdId::SET, num);
                ApiError e = cmdQueueSendAndWait(c, 100);
                if (e == ApiError::BUSY) busy++;
                else if (e == expectedFor(num)) ok++;
                else wrong++;
            }
        });
    }
    for (auto& x : th) x.join();
    char msg[96];
    snprintf(msg, sizeof msg, "ok=%d busy=%d wrong=%d", ok.load(), busy.load(), wrong.load());
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, wrong.load(), msg);           // never a foreign reply
    TEST_ASSERT_TRUE_MESSAGE(ok.load() >= T * N / 2, msg);         // and the path really works
                                                                   // (BUSY allowed: 8 callers share one 100 ms window)
}

void test_priority_before_normal() {
    Command a = mkCmd(CmdId::SET, 1), b = mkCmd(CmdId::INIT), s = mkCmd(CmdId::STOP);
    TEST_ASSERT_TRUE(cmdQueueSend(a));
    TEST_ASSERT_TRUE(cmdQueueSend(b));
    TEST_ASSERT_TRUE(cmdQueueSend(s));
    Command out;
    TEST_ASSERT_TRUE(cmdQueueReceive(out, true));
    TEST_ASSERT_TRUE(out.id == CmdId::STOP);
    TEST_ASSERT_TRUE(cmdQueueReceive(out, true));
    TEST_ASSERT_TRUE(out.id == CmdId::SET);
}

void test_normal_held_back_when_not_allowed() {
    Command a = mkCmd(CmdId::MOVE);
    TEST_ASSERT_TRUE(cmdQueueSend(a));
    Command out;
    TEST_ASSERT_FALSE(cmdQueueReceive(out, false));               // cycle has a driver request
    TEST_ASSERT_TRUE(cmdQueueReceive(out, true));
    TEST_ASSERT_TRUE(out.id == CmdId::MOVE);
}

void test_priority_overflow_is_latched_never_dropped() {
    for (int i = 0; i < CMD_PRIO_QUEUE_LEN; ++i) {                // fill the priority queue
        Command s = mkCmd(CmdId::STOP);
        TEST_ASSERT_TRUE(cmdQueueSend(s));
    }
    for (int i = 0; i < 10; ++i) {                                // overflow -> latch
        Command s = mkCmd(CmdId::STOP);
        TEST_ASSERT_TRUE(cmdQueueSend(s));
    }
    Command h = mkCmd(CmdId::HV_OFF, 0, CmdSource::LOCAL), z = mkCmd(CmdId::SHUTTER_CLOSE);
    TEST_ASSERT_TRUE(cmdQueueSend(h));
    TEST_ASSERT_TRUE(cmdQueueSend(z));
    Command n = mkCmd(CmdId::INIT);
    TEST_ASSERT_TRUE(cmdQueueSend(n));

    // ONE call returns everything that is pending (control task step 3a)
    Command got[CMD_PRIO_MAX_PER_CYCLE];
    int k = cmdQueueReceivePriority(got, CMD_PRIO_MAX_PER_CYCLE);
    TEST_ASSERT_EQUAL_INT(CMD_PRIO_QUEUE_LEN + 3, k);
    for (int i = 0; i < CMD_PRIO_QUEUE_LEN; ++i) TEST_ASSERT_TRUE(got[i].id == CmdId::STOP);
    const int L = CMD_PRIO_QUEUE_LEN;
    TEST_ASSERT_TRUE(got[L].id == CmdId::STOP);                   // 10 STOPs coalesced
    TEST_ASSERT_TRUE(got[L].source == CmdSource::API);            // origin per command
    TEST_ASSERT_TRUE(got[L + 1].id == CmdId::HV_OFF);
    TEST_ASSERT_TRUE(got[L + 1].source == CmdSource::LOCAL);
    TEST_ASSERT_TRUE(got[L + 2].id == CmdId::SHUTTER_CLOSE);
    TEST_ASSERT_TRUE(got[L + 2].source == CmdSource::API);
    for (int i = L; i < L + 3; ++i) {
        TEST_ASSERT_FALSE(got[i].want_reply);                     // latched: no reply
        TEST_ASSERT_TRUE(got[i].t_enq_us != 0);                   // latency still measurable
    }
    TEST_ASSERT_EQUAL_INT(0, cmdQueueReceivePriority(got, CMD_PRIO_MAX_PER_CYCLE));
    Command out;
    TEST_ASSERT_TRUE(cmdQueueReceive(out, true));                 // normal command afterwards
    TEST_ASSERT_TRUE(out.id == CmdId::INIT);
}

void test_receive_priority_leaves_normal_commands() {
    Command a = mkCmd(CmdId::MOVE), b = mkCmd(CmdId::SHUTTER_CLOSE);
    TEST_ASSERT_TRUE(cmdQueueSend(a));
    TEST_ASSERT_TRUE(cmdQueueSend(b));
    Command got[CMD_PRIO_MAX_PER_CYCLE];
    TEST_ASSERT_EQUAL_INT(1, cmdQueueReceivePriority(got, CMD_PRIO_MAX_PER_CYCLE));
    TEST_ASSERT_TRUE(got[0].id == CmdId::SHUTTER_CLOSE);
    Command out;
    TEST_ASSERT_TRUE(cmdQueueReceive(out, true));
    TEST_ASSERT_TRUE(out.id == CmdId::MOVE);
}

void test_priority_burst_from_many_threads_fully_served() {
    // 8 producers fire 50 priority commands each while a consumer drains with
    // cmdQueueReceivePriority every 0.5 ms: nothing may be lost, every kind arrives.
    std::atomic<bool> go{true};
    std::atomic<int> seen_stop{0}, seen_hv{0}, seen_sc{0}, calls{0}, max_k{0};
    std::thread cons([&] {
        Command got[CMD_PRIO_MAX_PER_CYCLE];
        while (go) {
            int k = cmdQueueReceivePriority(got, CMD_PRIO_MAX_PER_CYCLE);
            if (k > max_k) max_k = k;              // no Unity asserts inside threads
            for (int i = 0; i < k; ++i) {
                if (got[i].id == CmdId::STOP) seen_stop++;
                else if (got[i].id == CmdId::HV_OFF) seen_hv++;
                else if (got[i].id == CmdId::SHUTTER_CLOSE) seen_sc++;
            }
            calls++;
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });
    std::vector<std::thread> prod;
    std::atomic<int> refused{0};
    for (int t = 0; t < 8; ++t) prod.emplace_back([&, t] {
        const CmdId ids[] = {CmdId::STOP, CmdId::HV_OFF, CmdId::SHUTTER_CLOSE};
        for (int i = 0; i < 50; ++i) {
            Command c = mkCmd(ids[(t + i) % 3]);
            if (!cmdQueueSend(c)) refused++;
        }
    });
    for (auto& p : prod) p.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    go = false; cons.join();
    Command rest[CMD_PRIO_MAX_PER_CYCLE];
    TEST_ASSERT_EQUAL_INT(0, cmdQueueReceivePriority(rest, CMD_PRIO_MAX_PER_CYCLE));   // fully drained
    TEST_ASSERT_EQUAL_INT(0, refused.load());                     // priority send never refused
    TEST_ASSERT_TRUE(max_k.load() <= CMD_PRIO_MAX_PER_CYCLE);
    TEST_ASSERT_TRUE(seen_stop > 0 && seen_hv > 0 && seen_sc > 0);
    TEST_ASSERT_TRUE(seen_stop + seen_hv + seen_sc <= 400);        // coalescing only reduces
}

void test_mutex_busy_priority_still_delivered() {
    // Caller A waits for a reply that does not come (no consumer) and holds the mutex.
    std::thread a([] { Command c = mkCmd(CmdId::SET, 5); cmdQueueSendAndWait(c, 300); });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    Command s = mkCmd(CmdId::STOP);
    uint32_t t0 = millis();
    ApiError e = cmdQueueSendAndWait(s, 40);                      // B: mutex busy
    uint32_t dt = millis() - t0;
    TEST_ASSERT_EQUAL_INT((int)ApiError::BUSY, (int)e);
    TEST_ASSERT_TRUE(dt < 200);                                   // B did not wait for A
    Command out;
    TEST_ASSERT_TRUE(cmdQueueReceive(out, false));                // ... but STOP is queued
    TEST_ASSERT_TRUE(out.id == CmdId::STOP);
    TEST_ASSERT_FALSE(out.want_reply);
    a.join();
}

void test_normal_queue_full_returns_busy() {
    for (int i = 0; i < CMD_QUEUE_LEN; ++i) { Command c = mkCmd(CmdId::SET, i); TEST_ASSERT_TRUE(cmdQueueSend(c)); }
    Command c = mkCmd(CmdId::SET, 99);
    TEST_ASSERT_FALSE(cmdQueueSend(c));
    uint32_t t0 = millis();
    TEST_ASSERT_EQUAL_INT((int)ApiError::BUSY, (int)cmdQueueSendAndWait(c, 100));
    TEST_ASSERT_TRUE(millis() - t0 < 50);                         // no pointless waiting
}

void test_late_reply_of_timed_out_command_is_ignored() {
    Command c1 = mkCmd(CmdId::SET, 1);
    TEST_ASSERT_EQUAL_INT((int)ApiError::BUSY, (int)cmdQueueSendAndWait(c1, 20));   // no consumer yet
    startConsumer();                                               // now c1 is executed late
    for (int i = 2; i < 30; ++i) {
        Command c = mkCmd(CmdId::SET, i);
        ApiError e = cmdQueueSendAndWait(c, 100);
        TEST_ASSERT_EQUAL_INT((int)expectedFor(i), (int)e);
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_concurrent_callers_get_their_own_reply);
    RUN_TEST(test_priority_before_normal);
    RUN_TEST(test_normal_held_back_when_not_allowed);
    RUN_TEST(test_priority_overflow_is_latched_never_dropped);
    RUN_TEST(test_receive_priority_leaves_normal_commands);
    RUN_TEST(test_priority_burst_from_many_threads_fully_served);
    RUN_TEST(test_mutex_busy_priority_still_delivered);
    RUN_TEST(test_normal_queue_full_returns_busy);
    RUN_TEST(test_late_reply_of_timed_out_command_is_ignored);
    return UNITY_END();
}
