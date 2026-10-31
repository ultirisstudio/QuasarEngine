extern "C"
{
    // Address sanitizer
    const char* __asan_default_options()
    {
        return "check_initialization_order=1:strict_init_order=1:detect_stack_use_after_return=1:"
               "strict_string_checks=1:detect_leaks=1";
    }

    // Undefined behavior sanitizer
    const char* __ubsan_default_options()
    {
        return "print_stacktrace=1:halt_on_error=1";
    }

    // Thread sanitizer
    const char* __tsan_default_options()
    {
        return "second_deadlock_stack=1:halt_on_error=1";
    }

    const char* __tsan_default_suppressions()
    {
        return "race:__sanitizer::IsAccessibleMemoryRange\n";
    }

    // Leak sanitizer
    const char* __lsan_default_suppressions()
    {
        return "leak:libX11.so\n"
               "leak:libxcb.so\n"
               "leak:libwayland-client.so\n"
               "leak:libdbus-1.so\n"
               "leak:libGLX\n"
               "leak:libEGL\n"
               "leak:libgallium\n"
               "leak:_dri.so\n"
               "leak:libnvidia-\n"
               "leak:libgtk-3.so\n"
               "leak:libgdk-3.so\n"
               "leak:libglib-2.0.so\n"
               "leak:libgobject-2.0.so\n"
               "leak:libfontconfig.so\n";
    }
}
