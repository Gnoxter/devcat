# /dev/cat - the pet for your kernel


/dev/cat is a small kernel module which simulates a cat via a probalistic automata.
It eats, sleeps, snoozes, plays and runs around. Its actions are sometimes reflected in the kernel log.
Sometimes while running around it accidently knocks over process, cats be cats you know.


	devcat: *eats*
	devcat: *zzz*
	devcat: *zzz*
	devcat: *zzz*
	devcat: *zzz*
	devcat: *purrrrr*
	devcat: *zzz*
	devcat: *purrrrr*
	devcat: *runs to the other side of the address space*
	devcat: toppled over nginx [753]
	devcat: *runs to this side of the address space*
	devcat: *purrrrr*
	devcat: *zzz*

## Feeding and other interactions
You can have some interaction with the cat, but it mostly ignores you.

    echo "feed" > /dev/cat
	echo "pet" > /dev/cat
	echo "call" > /dev/cat


## BUILDING
The module needs the kernel headers at the default location. By default it doesn't SIGSTOPs random
processes this can by enabled by defining the LIVING_DANGEROUSLY macro. 

	make LIVING_DANGEROUSLY=1


Processes that have been knocked over by the cat can be revieved by sending them SIGCONT:
	kill -SIGCONT $pid

The cat doesn't touch pids lower than 500. It may topple over your vases, but not your whole house.

**Unless your day is going slow you shouldn't deploy this on your production servers.**

