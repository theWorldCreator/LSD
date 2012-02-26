#include <Python.h>



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "lsd.h"

#ifndef FALSE
#define FALSE 0
#endif /* !FALSE */

#ifndef TRUE
#define TRUE 1
#endif /* !TRUE */


/*----------------------------------------------------------------------------*/
/** Fatal error, print a message to standard-error output and exit.
 */
static void error(char * msg)
{
  fprintf(stderr,"%s\n",msg);
  exit(EXIT_FAILURE);
}


/*----------------------------------------------------------------------------*/
/*------------------------------ PGM image I/O -------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/** Skip white characters and comments in a PGM file.
 */
static void skip_whites_and_comments(FILE * f)
{
  int c;
  do
    {
      while(isspace(c=getc(f))); /* skip spaces */
      if(c=='#') /* skip comments */
        while( c!='\n' && c!='\r' && c!=EOF )
          c=getc(f);
    }
  while( c == '#' || isspace(c) );
  if( c != EOF && ungetc(c,f) == EOF )
    error("Error: unable to 'ungetc' while reading PGM file.");
}

/*----------------------------------------------------------------------------*/
/** Read a ASCII number from a PGM file.
 */
static int get_num(FILE * f)
{
  int num,c;

  while(isspace(c=getc(f)));
  if(!isdigit(c)) error("Error: corrupted PGM file.");
  num = c - '0';
  while( isdigit(c=getc(f)) ) num = 10 * num + c - '0';
  if( c != EOF && ungetc(c,f) == EOF )
    error("Error: unable to 'ungetc' while reading PGM file.");

  return num;
}

/*----------------------------------------------------------------------------*/
/** Read a PGM file into an double image.
    If the name is "-" the file is read from standard input.
 */
static double * read_pgm_image_double(int * X, int * Y, char * name)
{
  FILE * f;
  int c,bin;
  int xsize,ysize,depth,x,y;
  double * image;

  /* open file */
  if( strcmp(name,"-") == 0 ) f = stdin;
  else f = fopen(name,"rb");
  if( f == NULL ) error("Error: unable to open input image file.");

  /* read header */
  if( getc(f) != 'P' ) error("Error: not a PGM file!");
  if( (c=getc(f)) == '2' ) bin = FALSE;
  else if( c == '5' ) bin = TRUE;
  else error("Error: not a PGM file!");
  skip_whites_and_comments(f);
  xsize = get_num(f);            /* X size */
  if(xsize<=0) error("Error: X size <=0, invalid PGM file\n");
  skip_whites_and_comments(f);
  ysize = get_num(f);            /* Y size */
  if(ysize<=0) error("Error: Y size <=0, invalid PGM file\n");
  skip_whites_and_comments(f);
  depth = get_num(f);            /* depth */
  if(depth<=0) fprintf(stderr,"Warning: depth<=0, probably invalid PGM file\n");
  /* white before data */
  if(!isspace(c=getc(f))) error("Error: corrupted PGM file.");

  /* get memory */
  image = (double *) calloc( (size_t) (xsize*ysize), sizeof(double) );
  if( image == NULL ) error("Error: not enough memory.");

  /* read data */
  for(y=0;y<ysize;y++)
    for(x=0;x<xsize;x++)
      image[ x + y * xsize ] = bin ? (double) getc(f)
                                   : (double) get_num(f);

  /* close file if needed */
  if( f != stdin && fclose(f) == EOF )
    error("Error: unable to close file while reading PGM file.");

  /* return image */
  *X = xsize;
  *Y = ysize;
  return image;
}


static PyObject *
lsd_call(PyObject *self, PyObject *args, PyObject *keywds)
{
  double * image = NULL;
  int X = 0,Y = 0;
  double * segs;
  int n;
  int dim = 7;
  //int * region;
  int regX,regY;
  int i;
  PyObject *py_region = NULL, *py_reg_list = NULL;
  
  double scale, sigma_coef, quant, ang_th, log_eps, density_th;
  int n_bins;
  char *image_path = NULL;
  
  
  static char *kwlist[] = {"scale", "sigma_coef", "quant", 
							  "ang_th", "log_eps", "density_th", 
							  "n_bins", "image_path", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ddddddis", kwlist,
                                     &scale, &sigma_coef, &quant,
                                     &ang_th, &log_eps, &density_th,
                                     &n_bins, &image_path)) {
    return NULL;
  }

  /* read input file */
  image = read_pgm_image_double(&X,&Y,image_path);

  /* execute LSD */
  segs = LineSegmentDetection( &n, image, X, Y,
                               scale,
                               sigma_coef,
                               quant,
                               ang_th,
                               log_eps,
                               density_th,
                               n_bins,
                               //is_assigned(arg,"reg") ? &region : NULL,
                               NULL, // FIXME, what is it?
                               &regX, &regY );
                               
    py_reg_list = Py_BuildValue("[]");
    
    for(i=0;i<n;i++)
    {
	  py_region = Py_BuildValue("{s:d, s:d, s:d, s:d, s:d, s:d, s:d}",
						"x1", segs[i*dim], "y1", segs[i*dim+1],
						"x2", segs[i*dim+2], "y2", segs[i*dim+3],
						"width", segs[i*dim+4], "p", segs[i*dim+5],
						"NFA", segs[i*dim+6]);
      PyList_Append(py_reg_list, py_region);
    }
              
    return py_reg_list;
}

static PyMethodDef LSDMethods[] = {
    {"lsd",  (PyCFunction)lsd_call, METH_VARARGS | METH_KEYWORDS,
     "Main function"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initlsdPyAPI(void)
{
  (void) Py_InitModule("lsdPyAPI", LSDMethods);
}
