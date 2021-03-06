#include "par.h"
#include "xplot.h"
#include <X11/Xatom.h>
#include <X11/keysym.h>

/* self-documentation */
char *sdoc = 
"XIPICK - X IMAGE plot and pick of a uniformly-sampled function f(x1,x2)\n"
"\n"
"xipick n1= [optional parameters] <binaryfile\n"
"\n"
"X Functionality:\n"
"Button 1	Zoom with rubberband box\n"
"Button 2	Show mouse (x1,x2) coordinates while pressed\n"
"Button 3	Save as a key 				\n"
"Q key		Quit (can also use Motif Action button)\n"
"a key		Append current mouse (x1,x2) location to end of picks \n"
"i key		Insert current mouse (x1,x2) location into middle of picks \n"
"d key		Delete current mouse (x1,x2) location from picks \n"
"p key		Display picks \n"
"s key		Append saved mouse (x1,x2) picks to pick file \n"
"c key		Clear all saved mouse (x1,x2) picks \n"
"\n"
"Required Parameters:\n"
"n1                     number of samples in 1st (fast) dimension\n"
"\n"
"Optional Parameters:\n"
"d1=1.0                 sampling interval in 1st dimension\n"
"f1=d1                  first sample in 1st dimension\n"
"n2=all                 number of samples in 2nd (slow) dimension\n"
"d2=1.0                 sampling interval in 2nd dimension\n"
"f2=d2                  first sample in 2nd dimension\n"
"perc=100.0             percentile used to determine clip\n"
"clip=(perc percentile) clip used to determine bclip and wclip\n"
"bperc=perc             percentile for determining black clip value\n"
"wperc=100.0-perc       percentile for determining white clip value\n"
"bclip=clip             data values outside of [bclip,wclip] are clipped\n"
"wclip=-clip            data values outside of [bclip,wclip] are clipped\n"
"cmap=gray              gray, hue, or default colormaps may be specified\n"
"verbose=1              =1 for info printed on stderr (0 for no info)\n"
"xbox=50                x in pixels of upper left corner of window\n"
"ybox=50                y in pixels of upper left corner of window\n"
"wbox=550               width in pixels of window\n"
"hbox=700               height in pixels of window\n"
"x1beg=x1min            value at which axis 1 begins\n"
"x1end=x1max            value at which axis 1 ends\n"
"d1num=0.0              numbered tic interval on axis 1 (0.0 for automatic)\n"
"f1num=x1min            first numbered tic on axis 1 (used if d1num not 0.0)\n"
"n1tic=1                number of tics per numbered tic on axis 1\n"
"grid1=none             grid lines on axis 1 - none, dot, dash, or solid\n"
"label1=                label on axis 1\n"
"x2beg=x2min            value at which axis 2 begins\n"
"x2end=x2max            value at which axis 2 ends\n"
"d2num=0.0              numbered tic interval on axis 2 (0.0 for automatic)\n"
"f2num=x2min            first numbered tic on axis 2 (used if d2num not 0.0)\n"
"n2tic=1                number of tics per numbered tic on axis 2\n"
"grid2=none             grid lines on axis 2 - none, dot, dash, or solid\n"
"label2=                label on axis 2\n"
"labelfont=Erg14        font name for axes labels\n"
"title=                 title of plot\n"
"titlefont=Rom22        font name for title\n"
"labelcolor=blue        color for axes labels\n"
"titlecolor=red         color for title\n"
"gridcolor=blue         color for grid lines\n"
"style=seismic          normal (axis 1 horizontal, axis 2 vertical) or\n"
"labelfont=Erg14        font name for axes labels\n"
"                       seismic (axis 1 vertical, axis 2 horizontal)\n"
"mpicks=stderr          file to save mouse picks (picks will be appended to \n"
"			  end of the file. (default: standard error output) \n"
"mpcolor=red            color to display picks on screen  \n"
"pcard=XPICK            name of the card to write picks (1-5 characters)\n"
"ppos=1                 panel position of the picks (cdp if input cdp gather)\n"
"porder=0               order of picks to output(0=x1 x2; 1=x2 x1)	\n"
"x1pscale=1.            scale to apply to x1 picks before output to pick file\n"
"x2pscale=1.            scale to apply to x2 picks before output to pick file\n"
"NOTE: \n"
"picks output card format: \n"
"1---5---10---15---20---25---30---35---40---45---50---55---60---65---70 \n"
"pcard      ppos       p11  p21  p12  p22  p13  p23  p14  p24  p15  p25 \n"
"\n";

/* functions defined and used internally */
static void zoomBox (int x, int y, int w, int h, 
	int xb, int yb, int wb, int hb,
	int nx, int ix, float x1, float x2,
	int ny, int iy, float y1, float y2,
	int *nxb, int *ixb, float *x1b, float *x2b,
	int *nyb, int *iby, float *y1b, float *y2b);
static unsigned char *newInterpBytes (int n1in, int n2in, unsigned char *bin,
	int n1out, int n2out);
void xMouseLoc(Display *dpy, Window win, XEvent event, int style, Bool show,
	int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb);
void xMousePrint(Display *dpy, Window win, XEvent event, int style,
	FILE *mpicksfp, int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb,
	char *pcard, int ppos, int porder, float *x1picks, float *x2picks,
	float x1pscale, float x2pscale, int npicks, int fex);
void xMousePicks(Display *dpy, Window win, XEvent event, int style,
	char *mpcolor,
	FILE *mpicksfp, int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb,
	char *pcard, int ppos, int porder, float *x1picks, float *x2picks,
	float x1pscale, float x2pscale, int *npicks, GC gc, int pkey,
	int *savebg);

static Display *dpy;
static Window win;

main (argc,argv)
int argc; char **argv;
{
	int n1,n2,n1tic,n2tic,nfloats,
		i1,i2,grid1,grid2,style,
		n1c,n2c,i1beg,i1end,i2beg,i2end,i1c,i2c,
		nz,iz,i1step,i2step,verbose,
		xbox,ybox,wbox,hbox,
		xb,yb,wb,hb,
		x,y,width,height,
		i,j,nx,ny,nxb,nyb,ixb,iyb,
		imageOutOfDate,winwidth=-1,winheight=-1,
		showloc=0,porder,ppos,npicks=0,savebg,pkey,fex;
	float labelsize,titlesize,perc,clip,bperc,wperc,bclip,wclip,
		d1,f1,d2,f2,*z,*temp,zscale,zoffset,zi,
		x1beg,x1end,x2beg,x2end,
		x1min,x1max,x2min,x2max,
		d1num,f1num,d2num,f2num,
		x1begb,x1endb,x2begb,x2endb,
		*x1picks,*x2picks,x1pscale,x2pscale;
	unsigned char *cz,*czp,*czb,*czbp,*czbi=NULL;
	char *label1="",*label2="",*title="",
		*labelfont="Erg14",*titlefont="Rom22",
		*styles="seismic",*grid1s="none",*grid2s="none",
		*labelcolor="blue",*titlecolor="red",
		*gridcolor="blue",*cmap="gray",keybuf[256],*mpicks,
		*pcard="XPICK",*mpcolor="red";
	FILE *infp=stdin, *mpicksfp;
/*
	Display *dpy;
	Window win;
*/
	XEvent event;
	KeySym keysym;
	XComposeStatus keystat;
	XImage *image=NULL;
	GC gci;
	int scr;
	unsigned long black,white,pmin,pmax;

	/* initialize getpar */
	initargs(argc,argv);
	askdoc(1);

	/* get parameters describing 1st dimension sampling */
	if (!getparint("n1",&n1))
		err("Must specify number of samples in 1st dimension!\n");
	d1 = 1.0;  getparfloat("d1",&d1);
	f1 = d1;  getparfloat("f1",&f1);
	x1min = (d1>0.0)?f1:f1+(n1-1)*d1;
	x1max = (d1<0.0)?f1:f1+(n1-1)*d1;

	/* get parameters describing 2nd dimension sampling */
	if (!getparint("n2",&n2)) {
		if (fseek(infp,0L,2)!=0)
			err("must specify n2 if in a pipe!\n");
		nfloats = eftell(infp)/sizeof(float);
		efseek(infp,0L,0);
		n2 = nfloats/n1;
	}
	d2 = 1.0;  getparfloat("d2",&d2);
	f2 = d2;  getparfloat("f2",&f2);
	x2min = (d2>0.0)?f2:f2+(n2-1)*d2;
	x2max = (d2<0.0)?f2:f2+(n2-1)*d2;

	/* set up file to save mouse picks */
	if (!getparstring("mpicks", &mpicks)) {
		mpicksfp = stderr;
		fex = 0;
	} else {
		/* if file exist */
                if((mpicksfp = fopen(mpicks,"r"))!=NULL) {
                        fclose(mpicksfp);
                        mpicksfp = efopen(mpicks,"a");
                        fex = 1;
                } else {
                        mpicksfp = efopen(mpicks,"w");
                        fex = 0;
                }
	}
	if (!getparstring("pcard", &pcard)) pcard = "XPICK";
	if (!getparint("porder", &porder)) porder = 0;
	if (!getparint("ppos", &ppos)) ppos = 1;
	if (!getparfloat("x1pscale", &x1pscale)) x1pscale = 1.;
	if (!getparfloat("x2pscale", &x2pscale)) x2pscale = 1.;
	/* 256 picks maximum */
	x1picks = (float*) malloc(256*sizeof(float));
	x2picks = (float*) malloc(256*sizeof(float));

	/* read binary data to be plotted */
	nz = n1*n2;
	z = ealloc1float(nz);
	if (fread(z,sizeof(float),nz,infp)!=nz)
		err("error reading input file");

	/* if necessary, determine clips from percentiles */
	if (getparfloat("clip",&clip)) {
		bclip = clip;
		wclip = -clip;
	}
	if ((!getparfloat("bclip",&bclip) || !getparfloat("wclip",&wclip)) &&
		!getparfloat("clip",&clip)) {
		perc = 100.0;  getparfloat("perc",&perc);
		temp = ealloc1float(nz);
		for (iz=0; iz<nz; iz++)
			temp[iz] = z[iz];
		if (!getparfloat("bclip",&bclip)) {
			bperc = perc;	getparfloat("bperc",&bperc);
			iz = (nz*bperc/100.0);
			if (iz<0) iz = 0;
			if (iz>nz-1) iz = nz-1;
			qkfind(iz,nz,temp);
			bclip = temp[iz];
		}
		if (!getparfloat("wclip",&wclip)) {
			wperc = 100.0-perc;  getparfloat("wperc",&wperc);
			iz = (nz*wperc/100.0);
			if (iz<0) iz = 0;
			if (iz>nz-1) iz = nz-1;
			qkfind(iz,nz,temp);
			wclip = temp[iz];
		}
		free1float(temp);
	}
	verbose = 1;  getparint("verbose",&verbose);
	if (verbose) warn("bclip=%g wclip=%g",bclip,wclip);

	/* get colormap specification */
	getparstring("cmap",&cmap);

	/* get axes parameters */
	xbox = 50; getparint("xbox",&xbox);
	ybox = 50; getparint("ybox",&ybox);
	wbox = 550; getparint("wbox",&wbox);
	hbox = 700; getparint("hbox",&hbox);
	x1beg = x1min; getparfloat("x1beg",&x1beg);
	x1end = x1max; getparfloat("x1end",&x1end);
	d1num = 0.0; getparfloat("d1num",&d1num);
	f1num = x1min; getparfloat("f1num",&f1num);
	n1tic = 1; getparint("n1tic",&n1tic);
	getparstring("grid1",&grid1s);
	if (STREQ("dot",grid1s)) grid1 = DOT;
	else if (STREQ("dash",grid1s)) grid1 = DASH;
	else if (STREQ("solid",grid1s)) grid1 = SOLID;
	else grid1 = NONE;
	getparstring("label1",&label1);
	x2beg = x2min; getparfloat("x2beg",&x2beg);
	x2end = x2max; getparfloat("x2end",&x2end);
	d2num = 0.0; getparfloat("d2num",&d2num);
	f2num = 0.0; getparfloat("f2num",&f2num);
	n2tic = 1; getparint("n2tic",&n2tic);
	getparstring("grid2",&grid2s);
	if (STREQ("dot",grid2s)) grid2 = DOT;
	else if (STREQ("dash",grid2s)) grid2 = DASH;
	else if (STREQ("solid",grid2s)) grid2 = SOLID;
	else grid2 = NONE;
	getparstring("label2",&label2);
	getparstring("labelfont",&labelfont);
	labelsize = 18.0; getparfloat("labelsize",&labelsize);
	getparstring("title",&title);
	getparstring("titlefont",&titlefont);
	titlesize = 24.0; getparfloat("titlesize",&titlesize);
	getparstring("style",&styles);
	if (STREQ("normal",styles)) style = NORMAL;
	else style = SEISMIC;
	getparstring("titlecolor",&titlecolor);
	getparstring("labelcolor",&labelcolor);
	getparstring("gridcolor",&gridcolor);
	getparstring("mpcolor",&mpcolor);

	/* adjust x1beg and x1end to fall on sampled values */
	i1beg = NINT((x1beg-f1)/d1);
	i1beg = MAX(0,MIN(n1-1,i1beg));
	x1beg = f1+i1beg*d1;
	i1end = NINT((x1end-f1)/d1);
	i1end = MAX(0,MIN(n1-1,i1end));
	x1end = f1+i1end*d1;

	/* adjust x2beg and x2end to fall on sampled values */
	i2beg = NINT((x2beg-f2)/d2);
	i2beg = MAX(0,MIN(n2-1,i2beg));
	x2beg = f2+i2beg*d2;
	i2end = NINT((x2end-f2)/d2);
	i2end = MAX(0,MIN(n2-1,i2end));
	x2end = f2+i2end*d2;

	/* allocate space for image bytes */
	n1c = 1+abs(i1end-i1beg);
	n2c = 1+abs(i2end-i2beg);
	cz = ealloc1(n1c*n2c,sizeof(unsigned char));

	/* convert data to be imaged into signed characters */
	zscale = (wclip!=bclip)?255.0/(wclip-bclip):1.0e10;
	zoffset = -bclip*zscale;
	i1step = (i1end>i1beg)?1:-1;
	i2step = (i2end>i2beg)?1:-1;
	if (style==NORMAL) {
		for (i2c=0,i2=i2beg; i2c<n2c; i2c++,i2+=i2step) {
			czp = cz+n1c*n2c-(i2c+1)*n1c;
			for (i1c=0,i1=i1beg; i1c<n1c; i1c++,i1+=i1step) {
				zi = zoffset+z[i1+i2*n1]*zscale;
				if (zi<0.0) zi = 0.0;
				if (zi>255.0) zi = 255.0;
				*czp++ = (unsigned char)zi;
			}
		}
	} else {
		czp = cz;
		for (i1c=0,i1=i1beg; i1c<n1c; i1c++,i1+=i1step) {
			for (i2c=0,i2=i2beg; i2c<n2c; i2c++,i2+=i2step) {
				zi = zoffset+z[i1+i2*n1]*zscale;
				if (zi<0.0) zi = 0.0;
				if (zi>255.0) zi = 255.0;
				*czp++ = (unsigned char)zi;
			}
		}
	}
	free1float(z);
	
	/* initialize zoom box parameters */
	nxb = nx = (style==NORMAL ? n1c : n2c);
	nyb = ny = (style==NORMAL ? n2c : n1c);
	ixb = iyb = 0;
	czb = cz;
	x1begb = x1beg;  x1endb = x1end;
	x2begb = x2beg;  x2endb = x2end;

	/* connect to X server */
	if ((dpy=XOpenDisplay(NULL))==NULL)
		err("Cannot connect to display %s!\n",XDisplayName(NULL));
	scr = DefaultScreen(dpy);
	black = BlackPixel(dpy,scr);
	white = WhitePixel(dpy,scr);
	
	/* create window */
	win = xNewWindow(dpy,xbox,ybox,wbox,hbox,black,white,"xipick");

	/* if necessary, create private colormap with gray scale */
	if (STREQ(cmap,"gray")) {
		XSetWindowColormap(dpy,win,xCreateGrayColormap(dpy,win));
	} else if (STREQ(cmap,"hue")) {
		XSetWindowColormap(dpy,win,xCreateHueColormap(dpy,win));
	}
	
	/* determine min and max pixels from standard colormap */
	pmin = xGetFirstPixel(dpy);
	pmax = xGetLastPixel(dpy);
		
	/* make GC for image */
	gci = XCreateGC(dpy,win,0,NULL);
	
	/* set normal event mask */
	XSelectInput(dpy,win,
		StructureNotifyMask |
		ExposureMask |
		KeyPressMask |
		PointerMotionMask |
		ButtonPressMask |
		ButtonReleaseMask |
		Button1MotionMask |
		Button2MotionMask);
	
	/* map window */
	XMapWindow(dpy,win);
					
	/* determine good size for axes box */
	xSizeAxesBox(dpy,win,
		labelfont,titlefont,style,
		&x,&y,&width,&height);
	
	/* clear the window */
	XClearWindow(dpy,win);
	
	/* note that image is out of date */
	imageOutOfDate = 1;
	savebg = 1;

	/* main event loop */
	while(True) {
		XNextEvent(dpy,&event);

		/* if window was resized */
		if (event.type==ConfigureNotify &&
			(event.xconfigure.width!=winwidth ||
			 event.xconfigure.height!=winheight)) {
			winwidth = event.xconfigure.width;
			winheight = event.xconfigure.height;
							
			/* determine good size for axes box */
			xSizeAxesBox(dpy,win,
				labelfont,titlefont,style,
				&x,&y,&width,&height);
			/* clear background */
                        if(savebg) XSetWindowBackground(dpy,win,0L);
			
			/* clear the window */
			XClearWindow(dpy,win);
			
			/* note that image is out of date */
			imageOutOfDate = 1;

		/* else if window exposed */
		} else if (event.type==Expose) {
			
			/* clear all expose events from queue */
			while (XCheckTypedEvent(dpy,Expose,&event));
			
			/* if necessary, make new image */
			if (imageOutOfDate) {
				if (czbi!=NULL) free1(czbi);
				czbi = newInterpBytes(nxb,nyb,czb,
					width,height);
				if (image!=NULL) XDestroyImage(image);
				image = xNewImage(dpy,pmin,pmax,
					width,height,czbi);
				imageOutOfDate = 0;
			}

			/* clear background */
                        if(savebg) XSetWindowBackground(dpy,win,0L);
	
			/* draw image (before axes so grid lines visible) */
			XPutImage(dpy,win,gci,image,0,0,x,y,
				image->width,image->height);
			
			/* draw axes on top of image */
			xDrawAxesBox(dpy,win,
				x,y,width,height,
				x1begb,x1endb,0.0,0.0,
				d1num,f1num,n1tic,grid1,label1,
				x2begb,x2endb,0.0,0.0,
				d2num,f2num,n2tic,grid2,label2,
				labelfont,title,titlefont,
				labelcolor,titlecolor,gridcolor,
				style);

		/* else if key down */
		} else if (event.type==KeyPress) {

			XLookupString(&event,keybuf,0,&keysym,&keystat);
			/* add the pick to the pick-saved buffers */
			pkey = 99;
			if (keysym==XK_a) {
				pkey = 0;
			/* delete the pick */
			} else if (keysym==XK_d) {
				pkey = 1;
			/* insert the pick */
			} else if (keysym==XK_i) {
				pkey = 2;
			/* display the pick */
			} else if (keysym==XK_p) {
				pkey = 3;
			} else if (keysym==XK_c) {
				pkey = 5;
			}
			if (pkey<=5) {
				xMousePicks(dpy,win,event,style,
					mpcolor,
					mpicksfp, x,y,width,height,
					x1begb,x1endb,x2begb,x2endb,
					pcard,ppos,porder,x1picks,x2picks,
					x1pscale,x2pscale,&npicks,gci,pkey,
					&savebg);
			/* output the picks */
			} else if (keysym==XK_s) {
				xMousePrint(dpy,win,event,style,
					mpicksfp, x,y,width,height,
					x1begb,x1endb,x2begb,x2endb,
					pcard,ppos,porder,x1picks,x2picks,
					x1pscale,x2pscale,npicks,fex);
			} else if (keysym==XK_Q) {
			/* This is the exit from the event loop */
				break;
			} else {
				continue;
			}


		/* else if button down (1 == zoom, 2 == mouse tracking */
		} else if (event.type==ButtonPress) {
			/* if 1st button: zoom */
			if (event.xbutton.button==Button1) {

				savebg = 1;
				/* track pointer and get new box */
				xRubberBox(dpy,win,event,&xb,&yb,&wb,&hb);
			
				/* if new box has tiny width or height */
				if (wb<4 || hb<4) {
				
					/* reset box to initial values */
					x1begb = x1beg;
					x1endb = x1end;
					x2begb = x2beg;
					x2endb = x2end;
					nxb = nx;
					nyb = ny;
					ixb = iyb = 0;
					if (czb!=cz) free1(czb);
					czb = cz;
			
				/* else, if new box has non-zero width
				/* and height */
				} else {
			
					/* calculate new box parameters */
					if (style==NORMAL) {
					    zoomBox(x,y,width,height,
						    xb,yb,wb,hb,
						    nxb,ixb,x1begb,x1endb,
						    nyb,iyb,x2endb,x2begb,
						    &nxb,&ixb,&x1begb,&x1endb,
						    &nyb,&iyb,&x2endb,&x2begb);
					} else {
					    zoomBox(x,y,width,height,
						    xb,yb,wb,hb,
						    nxb,ixb,x2begb,x2endb,
						    nyb,iyb,x1begb,x1endb,
						    &nxb,&ixb,&x2begb,&x2endb,
						    &nyb,&iyb,&x1begb,&x1endb);
					}
			
					/* make new bytes in zoombox */
					if (czb!=cz) free1(czb);
					czb = ealloc1(nxb*nyb,
						sizeof(signed char));
					for (i=0,czbp=czb; i<nyb; i++) {
					    czp = cz+(iyb+i)*nx+ixb;
					    for (j=0; j<nxb; j++)
						    *czbp++ = *czp++; 
					}
				}
			
				/* clear background */
                        	if(savebg) XSetWindowBackground(dpy,win,0L);

				/* clear area and force an expose event */
				XClearArea(dpy,win,0,0,0,0,True);
			
				/* note that image is out of date */
				imageOutOfDate = 1;
		
			/* else if 2nd button down: display mouse coords */
			} else if (event.xbutton.button==Button2) {

				showloc = 1;
				xMouseLoc(dpy,win,event,style,showloc,
					  x,y,width,height,x1begb,x1endb,
					  x2begb,x2endb);

			/* else if 3rd button down: track and pick */
                        } else if (event.xbutton.button==Button3) {
				pkey = 0;
				xMousePicks(dpy,win,event,style,
					mpcolor,
					mpicksfp, x,y,width,height,
					x1begb,x1endb,x2begb,x2endb,
					pcard,ppos,porder,x1picks,x2picks,
					x1pscale,x2pscale,&npicks,gci,pkey,
					&savebg);


			} else {
				continue;
			}

		/* else if pointer has moved */
		} else if (event.type==MotionNotify) {
			
			/* if button2 down, show mouse location */
			if (showloc)
				xMouseLoc(dpy,win,event,style,True,
					x,y,width,height,
					x1begb,x1endb,x2begb,x2endb);

		/* else if button2 released, stop tracking */
		} else if (event.type==ButtonRelease &&
			   event.xbutton.button==Button2) {
			showloc = 0;
		}

	} /* end of event loop */

	/* close connection to X server */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

/* update parameters associated with zoom box */
static void zoomBox (int x, int y, int w, int h, 
	int xb, int yb, int wb, int hb,
	int nx, int ix, float x1, float x2,
	int ny, int iy, float y1, float y2,
	int *nxb, int *ixb, float *x1b, float *x2b,
	int *nyb, int *iyb, float *y1b, float *y2b)
{
	/* if width and/or height of box are zero, just copy values */
	if (wb==0 || hb==0) {
		*nxb = nx; *ixb = ix; *x1b = x1; *x2b = x2;
		*nyb = ny; *iyb = iy; *y1b = y1; *y2b = y2;
		return;		
	} 
	
	/* clip box */
	if (xb<x) {
		wb -= x-xb;
		xb = x;
	}
	if (yb<y) {
		hb -= y-yb;
		yb = y;
	}
	if (xb+wb>x+w) wb = x-xb+w;
	if (yb+hb>y+h) hb = y-yb+h;
	
	/* determine number of samples in rubber box (at least 2) */
	*nxb = MAX(nx*wb/w,2);
	*nyb = MAX(ny*hb/h,2);
	
	/* determine indices of first samples in box */
	*ixb = ix+(xb-x)*(nx-1)/w;
	*ixb = MIN(*ixb,ix+nx-*nxb);
	*iyb = iy+(yb-y)*(ny-1)/h;
	*iyb = MIN(*iyb,iy+ny-*nyb);
	
	
	/* determine box limits to nearest samples */
	*x1b = x1+(*ixb-ix)*(x2-x1)/(nx-1);
	*x2b = x1+(*ixb+*nxb-1-ix)*(x2-x1)/(nx-1);
	*y1b = y1+(*iyb-iy)*(y2-y1)/(ny-1);
	*y2b = y1+(*iyb+*nyb-1-iy)*(y2-y1)/(ny-1);
}

/* return pointer to new interpolated array of bytes */
static unsigned char *newInterpBytes (int n1in, int n2in, unsigned char *bin,
	int n1out, int n2out)
{
	unsigned char *bout;
	float d1in,d2in,d1out,d2out,f1in,f2in,f1out,f2out;
	
	f1in = f2in = f1out = f2out = 0.0;
	d1in = d2in = 1.0;
	d1out = d1in*(float)(n1in-1)/(float)(n1out-1);
	d2out = d2in*(float)(n2in-1)/(float)(n2out-1);
	bout = ealloc1(n1out*n2out,sizeof(unsigned char));
	intl2b(n1in,d1in,f1in,n2in,d2in,f2in,bin,
		n1out,d1out,f1out,n2out,d2out,f2out,bout);
	return bout;
}
	
void xMouseLoc(Display *dpy, Window win, XEvent event, int style, Bool show,
	int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb)
{
	XGCValues values;
	static XFontStruct *fs=NULL;
	static XCharStruct overall;
	static GC gc;
	int dummy,xoffset=10,yoffset=10;
	float x1,x2;
	char string[256];

	/* if first time, get font attributes and make gc */
	if (fs==NULL) {
		fs = XLoadQueryFont(dpy,"fixed");
		gc = XCreateGC(dpy,win,0,&values);
		XSetFont(dpy,gc,fs->fid);
		overall.width = 1;
		overall.ascent = 1;
		overall.descent = 1;
	}

	/* erase previous string */
	XClearArea(dpy,win,xoffset,yoffset,
		overall.width,overall.ascent+overall.descent,False);

	/* if not showing, then return */
	if (!show) return;

	/* convert mouse location to (x1,x2) coordinates */
	if (style==NORMAL) {
		x1 = x1begb+(x1endb-x1begb)*(event.xmotion.x-x)/width;
		x2 = x2endb+(x2begb-x2endb)*(event.xmotion.y-y)/height;
	} else {
		x1 = x1begb+(x1endb-x1begb)*(event.xmotion.y-y)/height;
		x2 = x2begb+(x2endb-x2begb)*(event.xmotion.x-x)/width;
	}

	/* draw string indicating mouse location */
	sprintf(string,"(%0.4g,%0.4g)",x1,x2);
	XTextExtents(fs,string,strlen(string),&dummy,&dummy,&dummy,&overall);
	XDrawImageString(dpy,win,gc,xoffset,yoffset+overall.ascent,
		string,strlen(string));
}

void xMousePrint(Display *dpy, Window win, XEvent event, int style,
	FILE *mpicksfp, int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb,
	char *pcard, int ppos, int porder, float *x1picks, float *x2picks,
	float x1pscale, float x2pscale, int npicks, int fex)
{
	int ic,ip,np; 
	int p1,p2;
	static int first=1;

	/* output picks */

	if(first==1 && fex==0) {
		first=999;
		fprintf(mpicksfp,
"1---5---10---15---20---25---30---35---40---45---50---55---60---65---70\n");
		fprintf(mpicksfp,
"pcard      ppos       px1  py1  px2  py2  px3  py3  px4  py4  px5  py5\n");
		fprintf(mpicksfp,"\n");
	}

	for (ic=0;ic<(npicks+4)/5;ic++) {
	   if( (ic+1)*5 < npicks ) {
	      np = 5;  
	   }
	   else { 
	      np = npicks - ic*5;
	   }
	   fprintf(mpicksfp, "%-5s%10d     ",pcard,ppos);
	   for(ip=0;ip<np;ip++) {
	      if(porder==0) {
	         p1 = x1picks[ic*5+ip]*x1pscale;	
	         p2 = x2picks[ic*5+ip]*x2pscale;	
	      } else {
	         p1 = x2picks[ic*5+ip]*x2pscale;	
	         p2 = x1picks[ic*5+ip]*x1pscale;	
	      }
	      fprintf(mpicksfp,"%5d%5d",p1,p2);
	   }
	   fprintf(mpicksfp,"\n");
	}
	fflush(mpicksfp);
}


void xMousePicks(Display *dpy, Window win, XEvent event, int style,
	char *mpcolor,
	FILE *mpicksfp, int x, int y, int width, int height,
	float x1begb, float x1endb, float x2begb, float x2endb,
	char *pcard, int ppos, int porder, float *x1picks, float *x2picks,
	float x1pscale, float x2pscale, int *npicks, GC gc, int pkey,
	int *savebg)
{
	float x1,x2;
	int ipicks;
	int xx1,yy1,xx2,yy2,temp,ip;
	int dismin,dis;
	int ipmin, ipins;
	GC gcp;
        XGCValues *values;
        XColor scolor,ecolor;
        XWindowAttributes wa;
        Colormap cmap;
        int scr;


	/* save bitmap of window to background for retrival */ 
	if(*savebg) {
		fg2bg(dpy,win);
		*savebg = 0;
	}

	/* convert mouse location to (x1,x2) coordinates */
	if (style==NORMAL) {
		x1 = x1begb+(x1endb-x1begb)*(event.xmotion.x-x)/width;
		x2 = x2endb+(x2begb-x2endb)*(event.xmotion.y-y)/height;
	} else {
		x1 = x1begb+(x1endb-x1begb)*(event.xmotion.y-y)/height;
		x2 = x2begb+(x2endb-x2begb)*(event.xmotion.x-x)/width;
	}

	/* save to x1picks and x2picks */
	if( pkey == 0 ) {
		ipicks = *npicks;
		x1picks[ipicks] = x1;
		x2picks[ipicks] = x2;
		*npicks = *npicks + 1;
	/* delete nearest picks from x1picks and x2picks*/
	} else if ( pkey==1 && *npicks > 0 ) {
		ipicks = *npicks;	
		dismin=(x1-x1picks[0])*(x1-x1picks[0])+
		       (x2-x2picks[0])*(x2-x2picks[0]);
		ipmin = 0;
		for(ip=1;ip<ipicks;ip++) {
		   	dis=(x1-x1picks[ip])*(x1-x1picks[ip])+
		       		(x2-x2picks[ip])*(x2-x2picks[ip]);
			if(dis<dismin) {
				dismin = dis;
				ipmin = ip;
			}
		}
		ipicks = ipicks - 1;
		for(ip=ipmin;ip<ipicks;ip++) {
			x1picks[ip] = x1picks[ip+1];
			x2picks[ip] = x2picks[ip+1];
		}
		*npicks = ipicks;
	/* insert the pick into x1picks and x2picks */
	} else if ( pkey==2 ) {
		ipicks = *npicks;	
		if (porder==0) {
			dismin=(x1-x1picks[0])*(x1-x1picks[0]);
			ipmin = 0;
			for(ip=1;ip<ipicks;ip++) {
		   		dis=(x1-x1picks[ip])*(x1-x1picks[ip]);
				if(dis<dismin) {
					dismin = dis;
					ipmin = ip;
				}
			}
			if(x1>x1picks[ipmin]) { 
				ipins = ipmin+1;
				ipicks +=1;
				for(ip=ipicks;ip>ipins;ip--) {
					x1picks[ip] = x1picks[ip-1];
					x2picks[ip] = x2picks[ip-1];
				}
				x1picks[ipins] = x1;
				x2picks[ipins] = x2;
			} else if(x1<x1picks[ipmin]) {
				ipins = ipmin;
				ipicks +=1;
				for(ip=ipicks;ip>ipins;ip--) {
					x1picks[ip] = x1picks[ip-1];
					x2picks[ip] = x2picks[ip-1];
				}
				x1picks[ipins] = x1;
				x2picks[ipins] = x2;
			}
		} else {
			dismin=(x2-x2picks[0])*(x2-x2picks[0]);
			ipmin = 0;
			for(ip=1;ip<ipicks;ip++) {
		   		dis=(x2-x2picks[ip])*(x2-x2picks[ip]);
				if(dis<dismin) {
					dismin = dis;
					ipmin = ip;
				}
			}
			if(x2>x2picks[ipmin]) { 
				ipins = ipmin+1;
				ipicks +=1;
				for(ip=ipicks;ip>ipins;ip--) {
					x1picks[ip] = x1picks[ip-1];
					x2picks[ip] = x2picks[ip-1];
				}
				x1picks[ipins] = x1;
				x2picks[ipins] = x2;
			} else if(x2<x2picks[ipmin]) {
				ipins = ipmin;
				ipicks +=1;
				for(ip=ipicks;ip>ipins;ip--) {
					x1picks[ip] = x1picks[ip-1];
					x2picks[ip] = x2picks[ip-1];
				}
				x1picks[ipins] = x1;
				x2picks[ipins] = x2;
			}
		}
		*npicks = ipicks;
	}
	if ( pkey==5 ) {
		for(ip=0;ip<*npicks;ip++) {
			x1picks[ip] = 0.;
			x2picks[ip] = 0.;
		}
		*npicks = 0;
		ipicks = 0;
	}

	ipicks = *npicks;

	/* draw lines between picks */
	if(ipicks>0) {
       		XClearWindow(dpy, win);
		/* get screen */
                scr = DefaultScreen(dpy);
                /* determine window's current colormap */
                XGetWindowAttributes(dpy,win,&wa);
                cmap = wa.colormap;
                gcp = XCreateGC(dpy,win,0,values);
                if (XAllocNamedColor(dpy,cmap,mpcolor,&scolor,&ecolor))
                        XSetForeground(dpy,gcp,ecolor.pixel);
                else
                        XSetForeground(dpy,gcp,1L);
                XSetLineAttributes(dpy,gcp,2,LineSolid,CapButt,JoinMiter);


		for(ip=1;ip<ipicks;ip++) {
			if (style==NORMAL) {
			xx1=(x1picks[ip-1]-x1begb)/(x1endb-x1begb)*width+x;
			yy1=(x2picks[ip-1]-x2endb)/(x2begb-x1endb)*height+y;
			xx2=(x1picks[ip]-x1begb)/(x1endb-x1begb)*width+x;
			yy2=(x2picks[ip]-x2endb)/(x2begb-x1endb)*height+y;
			} else {
			yy1=(x1picks[ip-1]-x1begb)/(x1endb-x1begb)*height+y;
			xx1=(x2picks[ip-1]-x2begb)/(x2endb-x2begb)*width+x;
			yy2=(x1picks[ip]-x1begb)/(x1endb-x1begb)*height+y;
			xx2=(x2picks[ip]-x2begb)/(x2endb-x2begb)*width+x;
			}
			XDrawLine(dpy,win,gcp,xx1,yy1,xx2,yy2);
		}
		/* free resources before returning */
                XFreeGC(dpy,gcp);
	}
}

