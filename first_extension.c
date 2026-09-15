struct kernel_services{
    void (*uart_puts)(const char* str);
};

void ext_main(struct kernel_services *ks)
{
    if(ks && ks->uart_puts)
    {
        ks->uart_puts("Hello I am extension 1 running");
    }
}