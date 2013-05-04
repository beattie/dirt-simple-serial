#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>

#define	CTRL_A	'\001'
#define CTRL_Q	'\021'
#define CTRL_X	'\030'

struct termios	serold, sernew;
struct termios	stdinold, stdinnew;
struct termios	stdoutold, stdoutnew;

int	fd = -1;
char	buffer[256];

char	*cmd;
char	portname[24];
int	speed;
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

int main(int argc, char *argv[])
{
	int	i, n, m, escape = 0, baud, mdmsig, exiting = 0;
	char	escapemsg[64];

	cmd = argv[0];
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <tty>\n", argv[0]);
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
			fprintf(stderr, "%s invalid baud rate %d\n", cmd, baud);
			clean_exit();
		}
	
		cfsetispeed(&sernew, baud);
		cfsetospeed(&sernew, baud);
	}

	tcsetattr(fd, TCSANOW, &sernew);
	tcsetattr(0, TCSANOW, &stdinnew);
	tcsetattr(1, TCSANOW, &stdoutnew);
	ioctl(fd, TIOCMBIC, TIOCM_RTS|TIOCM_DTR);
		
	for(;;)
	{
		settitle();
		if((n = read(fd, buffer, sizeof(buffer))) < 0)
		{
			if(errno != EAGAIN)
			{
				fprintf(stderr,
					"%s serial port read %d error (%s)\n",
					cmd, errno, strerror(errno));
				clean_exit();
			}
		}
		if(n > 0)
		{
			if(write(1, buffer, n) < n)
			{
				fprintf(stderr, "%s stdout write error "
					"(%s)\n", cmd, strerror(errno));
				clean_exit();
			}
		}
		if((n = read(0, buffer, sizeof(buffer))) < 0)
		{
			fprintf(stderr, "%s stdin read error (%s)\n",
				cmd, strerror(errno));
			clean_exit();
		}
		for(i = 0; i < n; i++)
		{
			/* look for escape char */
			if(! escape )
			{
				if((buffer[i]&127) == CTRL_A)
				{
					escape = 1;
					if(i > 0)
					{
						/*
					 	* write out what was before the
					 	* escape char
					 	*/
						if(write(fd, buffer, i) != i)
						{
							fprintf(stderr,
								"%s serial port"
								" write error "
								"(%s)\n", cmd,
								strerror(
									errno));
							clean_exit();
						}
					}
					i++;
					/*
				 	* subtract what has already
				 	* been sent and the escape
				 	* char.
				 	*/
					n -= i;
				}
			}
			if(!n)
			{
				break;
			}
			if(escape)
			{
				switch (buffer[i]&127)
				{
				case CTRL_A:
					escape = 0;
					memset(escapemsg, 0, sizeof escapemsg);
					i++;
					break;
				case 'f':
					/* disable hardware flow control */
					sernew.c_cflag &= ~CRTSCTS;
					tcsetattr(0, TCSANOW, &sernew);
					strcpy(escapemsg,
						"\n\rf disable flow "
						"control\n\r");
					n--;
					i++;
					break;
				case 'F':
					/* enable hardware flow control */
					sernew.c_cflag |= CRTSCTS;
					tcsetattr(0, TCSANOW, &sernew);
					strcpy(escapemsg,
						"\n\rF enable flow "
						"control\n\r");
					n--;
					i++;
					break;
				case 't':
					/* drop DTR */
					DTR = 0;
					mdmsig = TIOCM_DTR;
					ioctl(fd, TIOCMBIC, mdmsig);
					strcpy(escapemsg,
						"\n\rt drop DTR\n\r");
					n--;
					i++;
					break;
				case 'T':
					/* raise DTR */
					DTR = 1;
					mdmsig = TIOCM_DTR;
					ioctl(fd, TIOCMBIS, mdmsig);
					strcpy(escapemsg,
						"\n\rT raise DTR\n\r");
					n--;
					i++;
					break;
				case 'r':
					/* drop RTS/RTR */
					RTS = 0;
					if(sernew.c_cflag & CRTSCTS)
					{
						strcpy(escapemsg,
							"\n\rr drop RTS/RTS "
							"blocked by flow "
							"control\n\r");
						n--;
						i++;
						break;
					}
					mdmsig = TIOCM_RTS;
					ioctl(fd, TIOCMBIC, mdmsig);
					strcpy(escapemsg,
						"\n\rt drop RTS/RTR\n\r");
					n--;
					i++;
					break;
				case 'R':
					/* raise RTS/RTR */
					RTS = 1;
					if(sernew.c_cflag & CRTSCTS)
					{
						strcpy(escapemsg,
							"\n\rr raise RTS/RTS "
							"blocked by flow "
							"control\n\r");
						n--;
						i++;
						break;
					}
					mdmsig = TIOCM_RTS;
					ioctl(fd, TIOCMBIS, mdmsig);
					strcpy(escapemsg,
						"\n\rt raise RTS/RTR\n\r");
					n--;
					i++;
					break;
				case 'q':
				case 'Q':
				case 'x':
				case 'X':
				case CTRL_Q:
				case CTRL_X:
					exiting = 1;
					strcpy(escapemsg, "\n\rExit\n\r");
					n--;
					i++;
					break;
				case '?':
					fprintf(stdout,
						"\n\r*********************\n\r"
						"\n\r* ? - Print this    *\n\r"
						"\n\r*two ctrl-As, sens 1*\n\r"
						"\n\r* f - flow ctrl off *\n\r"
						"\n\r* F - flow ctrl on  *\n\r"
						"\n\r* t - drop DTR      *\n\r"
						"\n\r* T - raise DTR     *\n\r"
						"\n\r* r - drop RTS      *\n\r"
						"\n\r* R - raise RTS     *\n\r"
						"\n\r* Any of qQxX - Exit*\n\r"
						"\n\r* Also Ctrl-Q,Ctrl-X*\n\r"
						"\n\r*********************\n\r"
					);
					fflush(stdout);
				default:
					*escapemsg = '\0';
					break;
				}
				escape = 0;
				if(m = strlen(escapemsg))
				{
					write(1,escapemsg, m);
				}

			}
		}
		if(n)
		{
			if(write(fd, buffer, n) < n)
			{
				fprintf(stderr, "%s serial port write "
					"error (%s)\n", cmd,
					strerror(errno));
				clean_exit();
			}
		}
		if(exiting > 0)
		{
			fprintf(stdout, "\n\r%s Goodbye\n\r", cmd);
			clean_exit();
		}
	}
}

