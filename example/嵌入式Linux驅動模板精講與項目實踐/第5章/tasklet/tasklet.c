#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>   //attentions
#include <linux/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>

/*定义中断所用的结构体*/

struct button_irq_desc

{
    int irq; //按键对应的中断号
    int pin; //按键所对应的GPIO 端口
    int pin_setting; //按键对应的引脚描述，实际并未用到，保留
    int number; //定义键值，以传递给应用层/用户态
    char* name; //每个按键的名称
};

/*结构体实体定义*/
static struct button_irq_desc button_irqs [] = {
    {IRQ_EINT8 , S3C2410_GPG(0) , S3C2410_GPG0_EINT8 , 0, "KEY0"},
    {IRQ_EINT11, S3C2410_GPG(3) , S3C2410_GPG3_EINT11 , 1, "KEY1"},
    {IRQ_EINT13, S3C2410_GPG(5) , S3C2410_GPG5_EINT13 , 2, "KEY2"},
    {IRQ_EINT14, S3C2410_GPG(6) , S3C2410_GPG6_EINT14 , 3, "KEY3"},
    {IRQ_EINT15, S3C2410_GPG(7) , S3C2410_GPG7_EINT15 , 4, "KEY4"},
    {IRQ_EINT19, S3C2410_GPG(11), S3C2410_GPG11_EINT19, 5, "KEY5"},
};




static int  close(void) {
    int i;

    for (i = 0; i < sizeof(button_irqs) / sizeof(button_irqs[0]); i++) {
        if (button_irqs[i].irq < 0) {
            continue;
        }

        /*释放中断号，并注销中断处理函数*/
        free_irq(button_irqs[i].irq, (void*)&button_irqs[i]);
    }

    return 0;
}



static int times;
struct timer_list timer;

struct tasklet_struct time_tasklet;

//unsigned long t = 0;

wait_queue_head_t wait;

static int flag = 0;

void time_tasklet_funtion(unsigned long data) {

    //  while(flag%2);

    /*  t = jiffies;
      while(time_after(jiffies,t + 2*HZ)!=1);
    */
    //    sleep_on_timeout(&wait,2 * HZ);
    printk("tasklet data :%d,result:%d\n", data);


    //tasklet_schedule(&time_tasklet);
}


void timer_funtion(unsigned long para) {
    //    printk("time:\n");

    //   tasklet_schedule(&time_tasklet);
    mod_timer(&timer, jiffies + (HZ));
}


static irqreturn_t buttons_interrupt(int irq, void* dev_id) {
    printk("init flag:%d\n", flag);
    flag++;
    tasklet_schedule(&time_tasklet);
    return IRQ_RETVAL(IRQ_HANDLED);
}


static int  open(void) {
    int i;
    int err = 0;

    for (i = 0; i < sizeof(button_irqs) / sizeof(button_irqs[0]); i++) {
        if (button_irqs[i].irq < 0) {
            continue;
        }

        /*注册中断函数*/
        err = request_irq(button_irqs[i].irq, buttons_interrupt, IRQ_TYPE_EDGE_RISING,
                          button_irqs[i].name, (void*)&button_irqs[i]);

        if (err) {
            break;
        }
    }

    if (err) {

        /*如果出错，释放已经注册的中断，并返回*/

        i--;

        for (; i >= 0; i--) {
            if (button_irqs[i].irq < 0) {
                continue;
            }

            disable_irq(button_irqs[i].irq);
            free_irq(button_irqs[i].irq, (void*)&button_irqs[i]);
        }

        return -EBUSY;
    }

    /*正常返回*/
    return 0;
}


int __init timer_init(void) {

    open();
    init_timer(&timer);
    timer.data = times;
    timer.expires = jiffies + HZ;
    timer.function = timer_funtion;
    tasklet_init(&time_tasklet, time_tasklet_funtion, 0);
    add_timer(&timer);

    // tasklet_schedule(&time_tasklet);
    // init_waitqueue_head(&wait);
    return 0;
}


void __exit timer_exit(void) {
    close();
    del_timer(&timer);
    tasklet_kill(&time_tasklet);
    printk("<1>takslet has been destroyed\n");
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("lxlong");
MODULE_DESCRIPTION("tasklet test");
MODULE_ALIAS("tasklet module");
module_param(times, int, S_IRUGO);
MODULE_PARM_DESC(times, "timer set");


