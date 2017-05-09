extern int set_device(int minor, int pad);
extern int ib_read(int ud, uint max_num_bytes, char *buffer);
extern int ib_query(int ud, uint max_num_bytes, char *command, char *buffer);
extern int ib_write(int ud, char *command);

#define NODEBUG 
