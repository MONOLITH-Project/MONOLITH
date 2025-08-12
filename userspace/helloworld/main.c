static inline long syscall0(long num)
{
    long ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}

int main(int argc, char **argv)
{
    syscall0(0);
    while (1)
        ;
    return 0;
}
