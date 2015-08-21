/*
	Based on KML sample code by derekmolloy.ie
	Under GPL
*/

#include <linux/init.h> 
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "cat" 
#define CLASS_NAME  "feline"     
#define MINIMUM_DELAY_IN_MSEC 240000
#define RUNNING_DELAY_IN_MSEC 10000

#define STATE_COUNT 7

MODULE_LICENSE("GPL");   
MODULE_AUTHOR("gnxtr");
MODULE_DESCRIPTION("Moew");
MODULE_VERSION("42");

static int    majorNumber;
static struct class*  devcatClass  = NULL;
static struct device* devcatDevice = NULL;

static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .write = dev_write,
};

static DEFINE_MUTEX(devcat_mutex);
static struct timer_list devcat_timer;

//States of the cat automaton
typedef enum {
	S_EATING = 0,
	S_SLEEPING,
	S_DOZING,
	S_MAD,
	S_PLAYING,
	S_THIS_SIDE,
	S_THAT_SIDE,
} catdev_S;

//Strings for the different states
static char *states_string[] = {
	"*eats*",
	"*zzz*",
	"*purrrrr*",
	"*hiss*",
	"*plays*",
	"*runs to this side of the address space*",
	"*runs to the other side of the address space*"
};

//Input symbols for the automaton
typedef enum {
	I_TIMER = 0, //Special input symbol trigger by timer
	I_PET,
	I_CALL,
	I_FEED
} catdev_I;

/*
	This is the probability matrix for \delta
*/
static int Q[STATE_COUNT][4][STATE_COUNT] = {
{//EATING
	{0, 50, 255, 0, 0, 0, 0}, //TIMER
	{255, 0, 0, 0, 0, 0, 0}, //PET
	{255, 0, 0, 0, 0, 0, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0} //FEED
},
{//SLEEPING
	{0, 180, 220, 0, 240, 0, 255}, //TIMER
	{0, 0, 170, 200, 255, 0, 0}, //PET
	{0, 255, 0, 0, 0, 0, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0} //FEED
},
{//DOZING
	{0, 220, 125, 0, 240, 0, 255}, //TIMER
	{0, 0, 170, 200, 255, 0, 0}, //PET
	{0, 0, 255, 0, 0, 0, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0} //FEED
},
{//MAD
	{0, 220, 125, 0, 255, 0, 0}, //TIMER
	{0, 0, 0, 255, 0, 0, 0}, //PET
	{0, 0, 0, 255, 0, 0, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0} //FEED
},
{ //PLAYING
	{0, 0, 125, 0, 200, 0, 255}, //TIMER
	{0, 0, 125, 200, 255, 0, 0}, //PET
	{0, 0, 0, 0, 255, 0, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0}, //FEED
},
{ //RUNS THIS SIDE
	{0, 0, 50, 0, 100, 0, 255}, //TIMER
	{0, 0, 0, 125, 200, 0, 255}, //PET
	{0, 0, 0, 0, 0, 255, 0}, //CALL
	{255, 0, 0, 0, 0, 0, 0}, //FEED
},
{ //RUNS THAT SIDE
	{0, 0, 50, 0, 0, 255, 0}, //TIMER
	{0, 0, 0, 135, 250, 255, 0}, //PET
	{0, 0, 0, 0, 0, 0, 255}, //CALL
	{255, 0, 0, 0, 0, 0, 0}, //FEED
}

};

static catdev_S current_state = 0;
static int next_state(catdev_I input);
static void print_action50(void);
static void  devcat_callback(unsigned long data );


/*
	The transition function \delta: Q X I -> Q'
*/
static int next_state(catdev_I input) {
	int *matrix = Q[current_state][input];
	unsigned int random = 0, i;

	get_random_bytes(&random, sizeof(char));
	
	for (i = 0; i < STATE_COUNT; i++) {
		if (random <= matrix[i]) {
			current_state = i;
			return current_state;
		}
	}

	printk(KERN_CRIT "catdev is confused %i\n", current_state);
	current_state = S_SLEEPING;
	return current_state;
}

static void print_action50(void) {
	unsigned int random = 0;
	get_random_bytes(&random, sizeof(char));
	if (random > 125) {
		printk(KERN_INFO "devcat: %s\n", states_string[current_state]);
	}
}

static int maybe_true(void) {
	unsigned int random = 0;
	get_random_bytes(&random, sizeof(char));
	return random > 240;
}

static struct task_struct *get_process(void) {
	struct task_struct *t;
	unsigned int terminate = 0; 
	while (1) {
		for_each_process(t) {
			if (t->pid > 500 && (maybe_true() || terminate >= 10000)) {
				return t;
			}
			terminate++;
		}
	}
}

static void topple_over_process(void) {
	struct task_struct *flower_pot;

	flower_pot = get_process();

	send_sig(SIGSTOP, flower_pot, 0);
	printk("devcat: toppled over %s [%d]\n", flower_pot->comm, flower_pot->pid);

}

static void  devcat_callback(unsigned long data ) {
	int ret;
	mutex_lock(&devcat_mutex);
	current_state = next_state(I_TIMER);

	switch (current_state) {
		case 5:
			printk(KERN_INFO "devcat: %s\n", states_string[current_state]);
			ret = mod_timer(&devcat_timer, jiffies + msecs_to_jiffies(RUNNING_DELAY_IN_MSEC));
			break;
		case 6:
			printk(KERN_INFO "devcat: %s\n", states_string[current_state]);
#ifdef LIVING_DANGEROUSLY
			if (maybe_true()) {
				topple_over_process();
			}
#endif
			ret = mod_timer(&devcat_timer, jiffies + msecs_to_jiffies(RUNNING_DELAY_IN_MSEC));
			break;
		default:
			print_action50();
			ret = mod_timer(&devcat_timer, jiffies + msecs_to_jiffies(MINIMUM_DELAY_IN_MSEC));
	}

	mutex_unlock(&devcat_mutex);

	if (ret) {
		printk(KERN_ALERT "devcat: Error in mod_timer\n");
	}
}

static int __init devcat_init(void){
	// Register major number
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "devcat: failed to register a major number\n");
		return majorNumber;
	}

	// Register the device class
	devcatClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(devcatClass)) {
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "devcat: Failed to register device class\n");
		return PTR_ERR(devcatClass);
	}

	// Register the device driver
	devcatDevice = device_create(devcatClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(devcatDevice)) {
		class_destroy(devcatClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "devcat: Failed to create the device\n");
		return PTR_ERR(devcatDevice);
	}

	printk(KERN_INFO "devcat: Moew!\n"); 

	//Init Mutex & Timer
	mutex_init(&devcat_mutex);
	setup_timer(&devcat_timer, devcat_callback, 0 );

	//Lazy way to set the timer
	devcat_callback(0);

	return 0;
}

static void __exit devcat_exit(void) {
	//Deregister the Blockdevice
	device_destroy(devcatClass, MKDEV(majorNumber, 0));
	class_unregister(devcatClass);
	class_destroy(devcatClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);

	//Deregister timer and mutex
	del_timer_sync(&devcat_timer);
	mutex_destroy(&devcat_mutex);

	printk(KERN_INFO "devcat: awaaaaaaay\n");
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	mutex_lock(&devcat_mutex);
	if (strncmp("pet\n", buffer, len) == 0) {
		next_state(I_PET);
	} else if (strncmp("call\n", buffer, len) == 0) {
		next_state(I_CALL);
	} else if (strncmp("feed\n", buffer, len) == 0) {
		next_state(I_FEED);
	}

	printk(KERN_INFO "devcat: %s\n", states_string[current_state]);
	mutex_unlock(&devcat_mutex);
	return len;
}

module_init(devcat_init);
module_exit(devcat_exit);
