/* include the X library headers */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xft/Xft.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#include "drw.h"
#include "util.h"



#define LENGTH(X)             (sizeof X / sizeof X[0])
#define TEXTW(X)              (drw_fontset_getwidth(drw, (X)) + texth)

/* date +"%H:%M" */
/* date +"%A," */
/* date +"%d. %B 1%Y" */


/* X variables */

Display* 	dpy;
int 		screen;
Window 		win, root;
GC 			gc;

Drw* drw;

pthread_mutex_t globalmutex = PTHREAD_MUTEX_INITIALIZER;


int w, h;
int x, y;

int texth;
static volatile int running;

char* datestring[3];

enum {SchemeNorm, SchemeSel, SchemeLast};
enum {FontsNorm, FontsBig, FontsLast};


#include "config.h"


Clr* scheme[SchemeLast];
Fnt* fontsets[FontsLast];


/* Function Declarations */

void init_x(void);
void close_x(void);
void redraw(void);
void draw(void);
void cleanup(void);
void sig_handler(int sig);
void* updatedate_thr(void* arg);
int drw_strwrap(Drw* fdrw, const char* str, int pixelw, char*** line_r, int** length_r);



/* Function Definitions */

void
draw(void) {
	int bfh,nfh;

	{
		FILE* fs;
		char buf[256];
		int buflen;

		fs = popen("date +\"%H:%M\"", "r");
		fgets(buf, 256, fs);
		pclose(fs);
		buflen = strlen(buf);
		
		datestring[0] = (char*) realloc(datestring[0], sizeof(char) * buflen + 1);
		snprintf(datestring[0], buflen, "%s", buf);
		
		if (strcmp(buf, "00:00")) {
			fs = popen("date +\"%A,\"", "r");
			fgets(buf, 256, fs);
			pclose(fs);
			buflen = strlen(buf);
			
			datestring[1] = (char*) realloc(datestring[1], sizeof(char) * buflen + 1);
			snprintf(datestring[1], buflen, "%s", buf);

			fs = popen("date +\"%d. %B 1%Y\"", "r");
			fgets(buf, 256, fs);
			pclose(fs);
			buflen = strlen(buf);
			
			datestring[2] = (char*) realloc(datestring[2], sizeof(char) * buflen + 1);
			snprintf(datestring[2], buflen, "%s", buf);
		}
	}

	drw_rect(drw, 0, 0, w, h, 1, 0);

	bfh = fontsets[FontsBig]->h;
	nfh = fontsets[FontsNorm]->h;

	drw->fonts = fontsets[FontsBig];
	/* Writes Time in HH:MM format */
	drw_text(drw, 30, 150, 				w - 60, bfh, 0, datestring[0], 1);

	drw->fonts = fontsets[FontsNorm];
	/* Writes date in %A, \n %d. %B 1%Y */
	drw_text(drw, 30, 150+bfh+20, 		w - 60, nfh, 0, datestring[1], 1);
	drw_text(drw, 30, 150+bfh+20+nfh, 	w - 60, nfh, 0, datestring[2], 1);

	drw_map(drw, win, 0, 0, w, h);
}

void
init_x(void) {
	XWindowAttributes rwa;


	if (!XInitThreads())
		die("error when calling XInitThreads()");

	dpy = XOpenDisplay((char *)0);
   	screen = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);


	if (!XGetWindowAttributes(dpy, root, &rwa))
		die("failed to get root window attributes\n");

	h = rwa.height;
	w = 380;
	x = rwa.width - w;
	y = 0;


	drw = drw_create(dpy, screen, root, w, h);

	for (int i = 0; i < SchemeLast; i++)
		scheme[i] = drw_scm_create(drw, vegabarcolor[i], 2);

	for (int i = 0; i < FontsLast; i++)
		fontsets[i] = drw_fontset_create(drw, vegabarfonts[i], fontsperset);

	drw->fonts = fontsets[FontsNorm];
	drw_setscheme(drw, scheme[SchemeNorm]);


	texth = drw->fonts->h;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixel = scheme[SchemeNorm][ColBg].pixel,
		.event_mask = ExposureMask|VisibilityChangeMask
	};

	XClassHint ch = {"vegabar", "vegabar"};

	win = XCreateWindow(dpy, root, x, y, w, h, 0, CopyFromParent, CopyFromParent, CopyFromParent, CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XSetClassHint(dpy, win, &ch);
	XSelectInput(dpy, win, ExposureMask|ButtonPressMask);

    gc = XCreateGC(dpy, win, 0,0);

	drw_resize(drw, w, h);

	XClearWindow(dpy, win);
	XMapRaised(dpy, win);

	{
		FILE* fs;
		char buf[256];
		int buflen;

		fs = popen("date +\"%H:%M\"", "r");
		fgets(buf, 256, fs);
		printf("%s\n", buf);
		pclose(fs);
		buflen = strlen(buf);
		
		datestring[0] = (char*) malloc(sizeof(char) * buflen + 1);
		snprintf(datestring[0], buflen, "%s", buf);
		
		fs = popen("date +\"%A,\"", "r");
		fgets(buf, 256, fs);
		printf("%s\n", buf);
		pclose(fs);
		buflen = strlen(buf);
		
		datestring[1] = (char*) malloc(sizeof(char) * buflen + 1);
		snprintf(datestring[1], buflen, "%s", buf);

		fs = popen("date +\"%d. %B 1%Y\"", "r");
		fgets(buf, 256, fs);
		printf("%s\n", buf);
		pclose(fs);
		buflen = strlen(buf);
		
		datestring[2] = (char*) malloc(sizeof(char) * buflen + 1);
		snprintf(datestring[2], buflen, "%s", buf);
	}

}

void
close_x(void) {
	XFreeGC(dpy, gc);
	for (int i = 0; i < SchemeLast; i++)
		free(scheme[i]);
	drw_free(drw);
	XDestroyWindow(dpy,win);
	XCloseDisplay(dpy);	
	exit(1);				
}

void
redraw(void) {
	XClearWindow(dpy, win);
	drw_resize(drw, w, h);
	draw();
}

int
drw_strwrap(Drw* fdrw, const char* str, int pixelw, char*** line_r, int** length_r) {
	int allocated_lines;
	int lines;
	char** line;
	int * len;
	int tl;
	int l;
	int p;
	int close_word;
	int open_word;
	int cw;
	int i;

	if (str == NULL)
		return 0;

	tl = strlen(str);

	char* s = (char*) malloc(tl * sizeof(char));
	strcpy(s, str);

	if (s == NULL)
		return 0;

	if (tl == 0)
		return 0;


	allocated_lines = (tl / 30) * 1.5 + 1;

	line = (char**) malloc(sizeof(char*) * allocated_lines);
	len = (int*) malloc(sizeof(int) * allocated_lines);

	if (line == NULL || len == NULL)
		return 0;

	lines = 0;
	l = 0;
	p = 0;
	cw = 0; /* drw */

	while (p < tl) {
		l = 0;
		cw = 1;

		if (*(s+p) == '\n') {
			l = 0;
			goto make_new_line;
		}

		if (isspace(*(s+p))) {
			p++;
			continue;
		}

		char ss[500];
		memcpy(ss, s+p, cw);
		*(ss+cw+1) = '\0';

		while (drw_fontset_getwidth(fdrw, ss) < pixelw && cw < 500) {
			cw++;
			memcpy(ss, s+p, cw);
			*(ss+cw+1) = '\0';
		}
		cw--;

		if (cw <= 0)
			return 0;

		if (p + cw > tl)
			cw = tl - p;

		l = cw;

		close_word = 0;

		while (*(s+p+l+close_word) != '\0' && isspace(*(s+p+l+close_word)))
			close_word++;



		open_word = 0;

		while (*(s+p+l) != '\0' && !isspace(*(s+p+l))) {
			l--;
			open_word++;

			if (open_word + close_word > cw * 0.8){
				l = cw;
				break;
			}
		}

make_new_line:


		line[lines] = (s+p);
		len[lines] = l;
		lines++;

		if (lines >= allocated_lines) {
			allocated_lines *= 2;

			line = (char**) realloc(line, sizeof(char*) * allocated_lines);
			len = (int*) realloc(len, sizeof(int) * allocated_lines);

			if (line == NULL || len == NULL)
				return 0;
		}

		if (l == cw)
			l--;

		p += l + 1;

	}

	{
		char** strings = (char**) malloc(sizeof(char*) * lines);

		for (i = 0; i < lines; i++) {
			*(strings+i) = (char*) malloc(sizeof(char) * *(len+i));
			strncpy(*(strings+i), *(line+i), *(len+i));
			*(*(strings+i)+*(len+i)) = '\0';
		}

		*line_r = strings;
		free(line);
	}

	
	*length_r = len;

	return lines;
}

void
sig_handler(int sig) {
	if (sig == SIGINT)
		running = 0;
}

void
cleanup(void) {
	for (int i = 0; i < FontsLast; i++)
		drw_fontset_free(fontsets[i]);
	free(datestring[0]);
	free(datestring[1]);
	free(datestring[2]);
	close_x();
}

void*
updatedate_thr(void* arg) {
	time_t tt;
	struct tm t;


	while(running) {
		time(&tt);
		t = *localtime(&tt);
		sleep(60-t.tm_sec);
		pthread_mutex_lock(&globalmutex);
		redraw();
		pthread_mutex_unlock(&globalmutex);
	}

	return NULL;
}


/* main() */

int
main(int argc, char** argv) {
	pthread_t updatetime_thread;
	XEvent ev;

	{
		if (signal(SIGINT, sig_handler) == SIG_ERR)
			printf("cant catch SIGINT\n");
	}

	init_x();
	draw();

	/* look for events forever... */
	running = 1;
	if (pthread_create(&updatetime_thread, NULL, updatedate_thr, NULL))
		die("error creating thread.\n");

	while (running) {
		XNextEvent(dpy, &ev);
		if (XFilterEvent(&ev, None))
			continue;

		switch (ev.type) {
			case Expose:
				if (ev.xexpose.count == 0) {
					pthread_mutex_lock(&globalmutex);
					drw_map(drw, win, 0, 0, w, h);
					pthread_mutex_unlock(&globalmutex);
				}
				break;
			case VisibilityNotify:
				if (ev.xvisibility.state != VisibilityUnobscured) {
					pthread_mutex_lock(&globalmutex);
					XRaiseWindow(dpy, win);
					pthread_mutex_unlock(&globalmutex);
				}
		}
	}

	pthread_join(updatetime_thread, NULL);

	while (w > 1) {
		w -= 30;
		x += 30;
		if (w < 1)
			w = 1;
		XMoveResizeWindow(dpy, win, x, y, w, h);
		redraw();
	
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
	}

	cleanup();
}
