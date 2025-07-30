static __inline void outb(unsigned char __value, unsigned short int __port)
{
    __asm__ __volatile__("outb %b0,%w1" : : "a"(__value), "Nd"(__port));
}

void serial_print(const char *str)
{
    while (*str) {
        outb(*str, 0x3F8);
        str++;
    }
}

int main()
{
    serial_print("Hello World\n");
    return 0;
}
