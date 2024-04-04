// ldc doesn't expose these out of the box
pragma(LDC_intrinsic, "llvm.aarch64.crc32cx")
    uint __builtin_arm_crc32cd(uint, ulong) pure @safe @nogc nothrow;
pragma(LDC_intrinsic, "llvm.aarch64.crc32cw")
    uint __builtin_arm_crc32cw(uint, uint) pure @safe @nogc nothrow;
pragma(LDC_intrinsic, "llvm.aarch64.crc32ch")
    uint __builtin_arm_crc32ch(uint, uint) pure @safe @nogc nothrow;
pragma(LDC_intrinsic, "llvm.aarch64.crc32cb")
    uint __builtin_arm_crc32cb(uint, uint) pure @safe @nogc nothrow;

alias __crc32cd = __builtin_arm_crc32cd;
alias __crc32ch = __builtin_arm_crc32ch;
alias __crc32cw = __builtin_arm_crc32cw;
alias __crc32cb = __builtin_arm_crc32cb;
