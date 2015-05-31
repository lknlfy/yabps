#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <drv.h>
#include <stddef.h>     /* offset_of */
#include <zl/log.h>
#include <zl/types.h>
#include <assert.h>
#include <zl/util.h>
#include <pthread.h>

struct srl_t{
    drv_t drv;
    int fd;
};

static int printx(const char *promt,const u8 *x,size_t n) {
    static char buffer[1024];
    int l = sprintf(buffer,"%s(%02x) : ",promt,n);
    for(size_t i = 0;i < n && i < 1024;i++)
        l += sprintf(buffer + l,"%02x ",x[i]);
    LOGD("%s\n",buffer);
    return 0;
}


static int srl_configure(drv_t *__drv,speed_t rate) {
    struct srl_t *srl = container_of(__drv,struct srl_t,drv);
    struct termios options;
    int val;
    val = tcgetattr(srl->fd,&options);
    if(val < 0) {
        LOGD("srl_configure %s\n",strerror(errno));
        return -1;
    }
    cfsetispeed(&options,rate);
    cfsetospeed(&options,rate);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag |= (CLOCAL | CREAD);


	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~(ONLCR | OCRNL);

	options.c_iflag &= ~(ICRNL | INLCR);
    options.c_iflag &= ~INPCK;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	tcflush(srl->fd, TCIFLUSH);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;

    val = tcsetattr(srl->fd, TCSANOW, &options);
	if(val < 0) {
        LOGD("srl_configure %s\n",strerror(errno));
		return -1;
	}
    tcflush(srl->fd, TCIFLUSH);
    return 0;
}

static int srl_write(drv_t *__drv,int ch,const void *__buffer,size_t __n) {
    struct srl_t *srl = container_of(__drv,struct srl_t,drv);
    printx("TX",__buffer,__n);
    if(srl->fd > 0)
        return write(srl->fd,__buffer,__n);
    errno = -ENOENT;
    return -1;
}

static int srl_read(drv_t *__drv,void *__buffer,size_t __n) {
    struct srl_t *srl = container_of(__drv,struct srl_t,drv);
    if(srl->fd > 0)
        return read(srl->fd,__buffer,__n);
    errno = -ENOENT;
    return -1;
}

static int srl_close(drv_t *__drv) {
    struct srl_t *srl = container_of(__drv,struct srl_t,drv);
    if(srl->fd > 0)
        close(srl->fd);
    return 0;
}

static void* drv_reader(void *__prv) {
    static u8 buffer[1024];
    drv_t *drv = __prv;
    struct srl_t *srl = container_of(drv,struct srl_t,drv);
    int len;

    LOGD("drv_reader");
    while(1) {
        len = read(srl->fd,buffer,sizeof(buffer));
        if(len > 0) {
            int l = 0;
            printx("RX",buffer,len);
            while(l < len) {
                int il = drv_receiv(drv->top,buffer + l,len - l);
                if(il > 0)
                    l += il;
                else
                    break;
            }
        } else {
            LOGE("%s",strerror(errno));
        }
    }
    return NULL;
}

static int srl_start(drv_t *__drv) {
    //struct srl_t *srl = container_of(__drv,struct srl_t,drv);
    pthread_t pid;
    pthread_create(&pid,NULL,drv_reader,__drv);
    return 0;
}



drv_t *linux_serial(const char *path,drv_t *top) {
    struct srl_t *srl = __new(struct srl_t);
    assert(srl != NULL);
    assert(path != NULL);
    srl->fd = open(path,O_RDWR | O_NOCTTY);
    if(srl->fd < 0) {
        LOGE("%s %s",path,strerror(errno));
        delete(srl);
        return NULL;
    }
    srl->drv.configure = srl_configure;
    srl->drv.send = srl_write;
    srl->drv.start = srl_start;
    srl->drv.close = srl_close;
    srl->drv.top = top;
    return &(srl->drv);
}
