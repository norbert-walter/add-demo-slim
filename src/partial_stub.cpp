// Only for the compile check "pio run -e firmware-partial" of intermediate sprints (V1.5.15).
// The Arduino core calls setup()/loop(); until Sprint I1 writes src/main.cpp they do not exist,
// and --unresolved-symbols=ignore-all would put them at address 0, which the Xtensa linker
// rejects ("dangerous relocation: windowed longcall crosses 1GB boundary").
// Weak empty definitions: a real setup()/loop() always wins. Never flash a partial build.
#ifdef ADD_PARTIAL_BUILD
__attribute__((weak)) void setup() {}
__attribute__((weak)) void loop() {}
#endif
