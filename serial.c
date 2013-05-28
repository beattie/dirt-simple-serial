#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define	CTRL_A	'\001'
#define CTRL_Q	'\021'
#define CTRL_X	'\030'
#define CTRL_G	'\027'

#define SETCMD		'c'
#define LOGOFF		'l'
#define LOGON		'L'
#define	DROPDTR		't'
#define RAISEDTR	'T'
#define DROPRTS		'r'
#define RAISERTS	'R'
#define FLOWOFF		'f'
#define FLOWON		'F'
#define QUIT1		'q'
#define QUIT2		'Q'
#define QUIT3		CTRL_Q
#define QUIT4		'x'
#define QUIT5		'X'
#define QUIT6		CTRL_X
#define QUERY		'?'

char	cmd_esc = CTRL_G;

struct termios	serold, sernew;
struct termios	stdinold, stdinnew;
struct termios	stdoutold, stdoutnew;

int	fd = -1, logfd = -1, exiting = 0, flowcontrol;
char	buffer[2048];
char	escapemsg[64];

char	*cmd;
char	portname[24];
int	speed = -1;
int	DTR = 1;
int	RTS = 1;

void clean_exit(void)
{
	tcsetattr(0, TCSANOW, &stdinold);
	tcsetattr(0, TCSANOW, &stdoutold);
	if(fd < 0)
	{
		exit(-1);
	}
	tcsetattr(fd, TCSAFLUSH, &serold);
	exit(-1);
}

/*
 * create and send to the xterminal a title string with status info */
void settitle(void)
{
	struct termios	t;
	int		i, changed = 0;
	static int	mdmsig = 0, oldmdmsig = -1, FC = -1, oldspeed = -1;
	static int	olddtr = -1, oldrts = -1;
	static char	oldport[65];
	char	*p;
	char	escli[] = "\033]2;";
	char	port[65];
	char	esclo[] = "\007";

	if(speed != oldspeed)
	{
		changed = 1;
		oldspeed = speed;
	}
	if(DTR != olddtr)
	{
		changed = 1;
		olddtr = DTR;
	}
	if(RTS != oldrts)
	{
		changed = 1;
		oldrts = RTS;
	}
	tcgetattr(fd, &t);
	if(t.c_cflag != FC)
	{
		changed = 1;
		FC = t.c_cflag;
	}

	ioctl(fd, TIOCMGET, &mdmsig);
	if(mdmsig != oldmdmsig)
	{
		changed = 1;
		oldmdmsig = mdmsig;
	}
	memset(port, ' ', sizeof(port)-1);	/* clear port name field */
	port[sizeof(port)-1] = '\0';	/* null terminate string */
	if(strncmp("/dev/tty", portname, 8) == 0)
	{
		p = portname + 8;
	} else {
		p = portname;
	}
	for(i = 0; i < 64; i++)
	{
		if(p[i] == '\0')
		{
			break;
		}
		port[i] = p[i];
	}
	if(strcmp(port, oldport))
	{
		changed = 1;
		memcpy(oldport, port, sizeof port);
	}
	if(!changed)
	{
		return;
	}

	fprintf(stdout, "%sSerial %4s%4s%4s%4s%4s%5s%3s%7d %s%s", escli,
		RTS?"RTS ":"rts ",
		mdmsig&TIOCM_CTS?"CTS ":"cts ",
		mdmsig&TIOCM_DSR?"DSR ":"dsr ",
		mdmsig&TIOCM_CAR?"DCD ":"dcd ",
		DTR?"DTR ":"dtr ",
		"flow ", t.c_cflag&CRTSCTS?"on ":"off", speed, port, esclo);
	fflush(stdout);
}

void signal_exit(int sig)
{
	fprintf(stderr, "%s exited on signal %d\n", cmd, sig);
	clean_exit();
}

setattr(struct termios *t)
{
	/* clear bits */
	t->c_iflag &= ~(IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|INLCR|ICRNL|IXON|
		OPOST);
	/* set bits */
	t->c_iflag |= IXANY|IUTF8;

	/* clear bits */
	t->c_oflag &= ~(OLCUC|ONLCR|OCRNL|OFILL);

	/* start from zero bits */
	t->c_cflag = CS8|CREAD|CLOCAL;

	/* zero bits */
	t->c_lflag = 0;

	t->c_cc[VINTR] = 0;
	t->c_cc[VQUIT] = 0;
	t->c_cc[VERASE] = 0;
	t->c_cc[VKILL] = 0;
	t->c_cc[VEOF] = 0;
	t->c_cc[VEOL] = 0;
	t->c_cc[VEOL2] = 0;
	t->c_cc[VSWTC] = 0;
	t->c_cc[VSTART] = 0;
	t->c_cc[VSUSP] = 0;
#ifdef VDSUSP
	t->c_cc[VDSUSP] = 0;
#endif
	t->c_cc[VLNEXT] = 0;
	t->c_cc[VWERASE] = 0;
	t->c_cc[VDISCARD] = 0;
#ifdef VSTATUS
	t->c_cc[VSTATUS] = 0;
#endif

	/*
	 * if input queued return up th ethe number of byte requested.
	 * if no data queued wait for .2 seconds, return at least one
	 * byte if it arrives during time out, otherwise return zero bytes
	 */
	t->c_cc[VMIN] = 0;	/* minimum characters to read */
	t->c_cc[VTIME] = 2;	/* time out in tenths of seconds */
}

int checkspeed(int baud)
{
	switch(baud)
	{
	case 0:
		baud = B0;
		break;
	case 50:
		baud = B50;
		break;
	case 75:
		baud = B75;
		break;
	case 110:
		baud = B110;
		break;
	case 134:
		baud = B134;
		break;
	case 150:
		baud = B150;
		break;
	case 200:
		baud = B200;
		break;
	case 300:
		baud = B300;
		break;
	case 600:
		baud = B600;
		break;
	case 1200:
		baud = B1200;
		break;
	case 1800:
		baud = B1800;
		break;
	case 2400:
		baud = B2400;
		break;
	case 4800:
		baud = B4800;
		break;
	case 9600:
		baud = B9600;
		break;
	case 19200:
		baud = B19200;
		break;
	case 38400:
		baud = B38400;
		break;
	case 57600:
		baud = B57600;
		break;
	case 115200:
		baud = B115200;
		break;
	case 230400:
		baud = B230400;
		break;
	case 460800:
		baud = B460800;
		break;
	case 500000:
		baud = B500000;
		break;
	case 576000:
		baud = B576000;
		break;
	case 921600:
		baud = B921600;
		break;
	case 1000000:
		baud = B1000000;
		break;
	case 1152000:
		baud = B1152000;
		break;
	default:
		return -1;
	}
		return baud;
}

int openport(char *str)
{
	int		fd;
	const char	prepend[] = "/dev/tty";

	strncpy(portname, str, sizeof(portname) - 1);
	if((fd = open(portname, O_RDWR | O_EXCL | O_NOCTTY | O_NDELAY)) >= 0 )
	{
		return fd;
	}
	/* try adding /dev/ */
	memcpy(portname, prepend, 5);
	strncpy(portname + 5, str, sizeof(portname) - 6);
	if((fd = open(portname, O_RDWR | O_EXCL | O_NOCTTY | O_NDELAY)) >= 0 )
	{
		return fd;
	}
	/* try adding /dev/tty */
	strcpy(portname, prepend);
	strncpy(portname + strlen(prepend), str, sizeof(portname) -
		(strlen(prepend) + 1));
	if((fd = open(portname, O_RDWR | O_EXCL | O_NOCTTY | O_NDELAY)) >= 0 )
	{
		return fd;
	}
	fprintf(stderr, "%s can't open tty %s (%s)\n", cmd, portname,
		strerror(errno));
	return -1;
}

int do_command(char **p, int *n)
{
	int		i, mdmsig = 0;
	static char	*logname, template[]="/tmp/serial.XXXXXX.log";

	switch(**p)
	{
	case SETCMD:
		if(*n > 0)
		{
			cmd_esc = **p; 
			*p++;
			*n--;
		} else {
			if(read(fd, &cmd_esc, 1) != 1)
			{
				clean_exit();
			}
		}
		i = 1;
		break;
	case LOGOFF:
		if(logfd >= 0)
		{
			fprintf(stdout, "\n\r closing logfile: %s\n\r",
				logname);
			close(logfd);
			logfd = -1;
		}
		i = 1;
		break;
	case LOGON:
		if(logfd >= 0)	/* switch logfiles */
		{
			fprintf(stdout,
				"\n\r switching logfiles closing %s\n\r",
				logname);
			close(logfd);
			logfd = -1;
		}
		logname = malloc(strlen(template) + 1);
		strcpy(logname, template);
		if((logfd = mkostemps(logname, 4, O_WRONLY | O_APPEND |
					O_CREAT | O_EXCL)) < 0)
		{
			fprintf(stdout,
				"\n\rLogging: failed to open logfile\n\r");
		} else {
			fprintf(stdout, "\n\rLogging to:%s\n\r", logname);
			fchmod(logfd, 0644);
		}
		i = 1;
		break;
	case DROPDTR:
		/* drop DTR */
		DTR = 0;
		mdmsig = TIOCM_DTR;
		ioctl(fd, TIOCMBIC, mdmsig);
		strcpy(escapemsg, "\n\rt drop DTR\n\r");
		i = 1;
	case RAISEDTR:
		/* raise DTR */
		DTR = 1;
		mdmsig = TIOCM_DTR;
		ioctl(fd, TIOCMBIS, mdmsig);
		strcpy(escapemsg, "\n\rt drop DTR\n\r");
		i = 1;
	case DROPRTS:
		if(!flowcontrol)
		{
			strcpy(escapemsg, "\n\rr drop RTS/RTS "
				"blocked by flow control\n\r");
		} else {
			mdmsig = TIOCM_RTS;
			ioctl(fd, TIOCMBIC, mdmsig);
			strcpy(escapemsg,
				"\n\rt drop RTS/RTR\n\r");
		}
		i = 1;
		break;
	case RAISERTS:
		if(!flowcontrol)
		{
			strcpy(escapemsg, "\n\rr raise RTS/RTR "
				"blocked by flow control\n\r");
		} else {
			mdmsig = TIOCM_RTS;
			ioctl(fd, TIOCMBIC, mdmsig);
			strcpy(escapemsg,
				"\n\rt drop RTS/RTR\n\r");
		}
		i = 1;
		break;
	case FLOWOFF:
		/* disable hardware flow control */
		sernew.c_cflag &= ~CRTSCTS;
		tcsetattr(0, TCSANOW, &sernew);
		strcpy(escapemsg, "\n\rf disable flow "
			"control\n\r");
		flowcontrol = 0;
		i = 1;
		break;
	case FLOWON:
		/* enable hardware flow control */
		sernew.c_cflag |= CRTSCTS;
		tcsetattr(0, TCSANOW, &sernew);
		strcpy(escapemsg, "\n\rf disable flow "
			"control\n\r");
		flowcontrol = 0;
		i = 1;
		break;
	case QUIT1:
	case QUIT2:
	case QUIT3:
	case QUIT4:
	case QUIT5:
	case QUIT6:
		exiting = 1;
		strcpy(escapemsg, "\n\rGoodbye\n\r");
		clean_exit();
		i = 1;
		break;
	case QUERY:
		fprintf(stdout, "\n\r"
			"******************************\n\r"
			"* ? - Print this             *\n\r"
			"* two cmd escapes, sends 1   *\n\r"
			"* c - set cmd esc char       *\n\r"
			"* l - disable logging        *\n\r"
			"* L - enable logging (makes  *\n\r"
			"*     temporary file in /tmp *\n\r"
			"* f - flow ctrl off          *\n\r"
			"* F - flow ctrl on           *\n\r"
			"* t - drop DTR               *\n\r"
			"* T - raise DTR              *\n\r"
			"* r - drop RTS               *\n\r"
			"* R - raise RTS              *\n\r"
			"* Any of qQxX - Exit         *\n\r"
			"* Also Ctrl-Q,Ctrl-X         *\n\r"
			"******************************\n\r"
		);
		i = 1;
		break;
	default:
		i = 0;
		break;
	}
	return i;
}
int main(int argc, char *argv[])
{
	int		i, n, m, baud;
	char		*p, cmdchr = '\0', *cmdstart;
	fd_set		read_fds, write_fds, except_fds;
	struct timeval	tv;

	cmd = argv[0];
	if(argc < 2)
	{
		fprintf(stderr,
		"This program (dirt-simple-serial) or just serial is a\n"
		"serial port program, that is to say it is a program that\n"
		"is designed to connect to a serial port on a Linux\n"
		"computer that is connected to some external device such\n"
		"as a development board.  It allows the user to type\n"
		"commands to the development board.  To interface with the\n"
		"boot loader or to log into the console.  I can also be used\n"
		"to connect to a number of devices such as GPS modules that\n"
		"provide serial interfaces.\n\n"
		"the primary design goals were to be a simple as possible.\n"
		"To provide as unimpeded access to the serial device d to\n"
		"avoid unecessary features.  For instance with gnome-terminal\n"
		"or xterm process of displaying characters, providing support\n"
		"for editors such as emacs or vim are well handled by the\n"
		"terminal program.  While logging could have been done with\n"
		"the screen program I did add support for logging since\n"
		"turning screen on an off is not convienient. Finally,\n"
		"instead of providing a status line I ave hijacked the\n"
		"title feature of gnome-terminal/xterm to display that\n"
		"information.  The one feature that I may add is the ability\n"
		"run external programs with the serial port so file transfer\n"
		"programs can be run\n"
		"to exit the program enter the character sequence CTRL-Gq\n"
		"Q,CTRL-Q,x,X and CTRL-X will also work in the place of q.\n"
		"Enter the sequence CTRL-G? to get a list of commands\n\n");
		fprintf(stderr, "Usage: %s <tty> [speed]\n", argv[0]);
		exit(0);
	}
	signal(SIGTERM, signal_exit);
	signal(SIGHUP, signal_exit);
	signal(SIGINT, signal_exit);

	if((fd = openport(argv[1])) < 0)
	{
		exit -1;
	}

	tcgetattr(fd, &serold);
	tcgetattr(0, &stdinold);
	tcgetattr(1, &stdoutold);

	memcpy(&sernew, &serold, sizeof serold);
	memcpy(&stdinnew, &stdinold, sizeof stdinold);
	memcpy(&stdoutnew, &stdoutold, sizeof stdoutold);

	setattr(&sernew);
	setattr(&stdinnew);
	setattr(&stdoutnew);

	if(argc > 2)
	{
		speed = atoi(argv[2]);
		if((baud = checkspeed(speed)) < 0)
		{
			fprintf(stderr, "%s unsupported baud rate %d\n", cmd,
				speed);
			clean_exit();
		}
	
		cfsetispeed(&sernew, baud);
		cfsetospeed(&sernew, baud);
	}

	tcsetattr(fd, TCSANOW, &sernew);
	tcsetattr(0, TCSANOW, &stdinnew);
	tcsetattr(1, TCSANOW, &stdoutnew);
	ioctl(fd, TIOCMBIC, TIOCM_RTS|TIOCM_DTR);
		
	fprintf(stdout, "Connected %s@%d Ctrl-G? for help Ctrl-Gq to quit\n\r",
		portname, speed);
	for(;;)
	{
		settitle();
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		FD_ZERO(&except_fds);
		FD_SET(0, &read_fds);
		FD_SET(0, &except_fds);
		FD_SET(1, &write_fds);
		FD_SET(1, &except_fds);
		FD_SET(fd, &read_fds);
		FD_SET(fd, &write_fds);
		FD_SET(fd, &except_fds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		/*
		 * no time out  the three file descriptors are 0, 1 and
		 * fd can't be lower than 0 and 1, therefore it's the
		 * higest.
		 */
		if(select(fd + 1, &read_fds, NULL, &except_fds, &tv) < 0)
		{
			clean_exit();
		}
		/* process serial port to display */
		if(FD_ISSET(fd, &read_fds))
		{
			if((n = read(fd, buffer, sizeof(buffer))) > 0)
			{
				if(write(1, buffer, n) != n)
				{
					clean_exit();
				}
			} else {
				clean_exit();
			}
		}
		/*
		 * process keyboard (or pipe?) to serial port detect
		 * and process commands
		 */
		if(FD_ISSET(0, &read_fds) == 0)
		{
			continue;
		}
		if((n = read(0, buffer, sizeof(buffer))) < 0)
		{
			clean_exit();
		}
		if(!cmdchr)	/* not currently processing a command */
		{
			/* scan buffer for command */
			if((p = memchr(buffer, cmd_esc, n)) == NULL)
			{	/* no command */
				if(write(fd, buffer, n) != n)
				{
					clean_exit();
				}
				continue;
			}
			cmdchr = *p;
			cmdstart = p++;
			if((cmdstart - buffer) <= 0)
			{	/* end of buffer, we need more input */
				/* write the buffer minus the command */
				if(cmdstart > 0)
				{
					if(write(fd, buffer, n - 1) != (n - 1))
					{
						clean_exit();
					}
				}
				continue;
			}
			m = -1;
			if(*p != cmd_esc)
			{
				if(m)
				{
					i = (cmdstart - buffer) + m;
					n -= i;	/* # bytes following cmd */
				}
				m = do_command(&p, &n);
			} else {	/* two cmd esc's sends one */
				i = cmdstart - buffer;
				n -= i;	/* number of bytes following cmd esc */
			}
			if(m == 0 && n > 0)
			{ /* no command  write whole buffer */
				if(write(fd, buffer, n) != n)
				{
					clean_exit();
				}
				cmdchr = '\0';
				continue;
			}
			/* write the dataq up to the command esc */
			if(write(fd, buffer, cmdstart - buffer) !=
				(cmdstart - buffer))
			{
				clean_exit();
			}
			if(n > 0)
			{
				if(write(fd, p, n) != n)
				{
					clean_exit();
				}
			}
			cmdchr = '\0';
			continue;
		} else {	/* currently processing a comand */
				/* we have the command escape */
			fprintf(stderr, "\n\r currently processing "
				"command\n\r");
			if(buffer[0] == cmd_esc)
			{
				/* 
				 * a second cmd esc, drop the first one
				 * send the rest
				 */
				if(write(fd, buffer, n) != n)
				{
					clean_exit();
				}
				cmdchr = '\0';
				continue;
			}
			p = buffer;
			m = do_command(&p, &n);
			if(m = 0 && n < 0)
			{	/*
				 * no command, send the cmd escape and the
				 * whole buffer.
				 */
				if((write(fd, &cmdchr, 1) != 1) || (write(fd,
						p, n) != n))
				{
					clean_exit();
				}
			} else {
				/*
				 * we had a command, drop the command escape
				 * skip the command char
				 */
				if(write(fd, buffer + m, n - m) != (n - m))
				{
					clean_exit();
				}
			}
			cmdchr = '\0';
			continue;
		}
	}
}
