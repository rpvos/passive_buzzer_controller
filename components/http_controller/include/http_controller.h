#if !defined(http_controller)
#define http_controller

// Method to initialize the server
void http_controller_init();
// Method to set the callback
void set_frequency_callback(void (*cb)(int));

#endif // http_controller

