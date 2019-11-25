
struct disp_buffer {
	unsigned char *base;
	unsigned int physical;
	unsigned int size;

	void *handle;
	int mapfd;
};

int disp_buffer_init(void);
int disp_buffer_exit(void);
int disp_buffer_alloc(struct disp_buffer *buf);
int disp_buffer_free(struct disp_buffer *buf);
int disp_buffer_flush(void *address, int size);
