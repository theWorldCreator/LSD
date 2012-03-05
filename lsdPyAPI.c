#include <Python.h>



#include <float.h>
#include "lsd_io.c"

#include <Python.h>
#include "structmember.h"

typedef struct {
    PyObject_HEAD
    PyObject *regions;
    image_double angles;
    image_double modgrad;
    double logNT;
    double prec;
    double p;
    double scale;
} LSD;

static void
LSD_dealloc(LSD* self)
{
	Py_XDECREF(self->regions);
    free_obj_memory(&self->angles, &self->modgrad);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
LSD_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LSD *self;

    self = (LSD *)type->tp_alloc(type, 0);
    if (self != NULL) {
		self->regions = Py_BuildValue("[]");
        if (self->regions == NULL)
          {
            Py_DECREF(self);
            return NULL;
          }
        self->angles = NULL;
        self->modgrad = NULL;
        self->logNT = -1;
        self->prec = -1;
        self->p = -1;
    }

    return (PyObject *)self;
}

static int
LSD_init(LSD *self, PyObject *args, PyObject *keywds)
{
    double * image = NULL;
  int X = 0,Y = 0;
  double * segs;
  int n;
  int dim = 7;
  //int * region;
  int regX,regY;
  int i;
  PyObject *py_region = NULL;
  
  double scale = -1, sigma_coef = -1, quant = -1, ang_th = -1, log_eps = -DBL_MAX, density_th = -1;
  int n_bins = -1;
  char *image_path = NULL;
  
  
  static char *kwlist[] = {"scale", "sigma_coef", "quant", 
							  "ang_th", "log_eps", "density_th", 
							  "n_bins", "image_path", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ddddddis", kwlist,
                                     &scale, &sigma_coef, &quant,
                                     &ang_th, &log_eps, &density_th,
                                     &n_bins, &image_path)) {
    return -1; 
  }
  if (scale < 0.0) {
	  scale = 0.8;
  }
  if (sigma_coef < 0.0) {
	  sigma_coef = 0.6;
  }
  if (quant < 0.0) {
	  quant = 2.0;
  }
  if (ang_th < 0.0 || ang_th > 180.0) {
	  ang_th = 22.5;
  }
  //log_eps is too small for normal work
  if (log_eps < -DBL_MAX / 2.0) {
	  log_eps = 0;
  }
  if (density_th < 0.0 || ang_th > 1.0) {
	  density_th = 0.7;
  }
  if (n_bins < 1) {
	  n_bins = 1024;
  }
  
  self->scale = scale;

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
						   &regX, &regY,
						   &self->angles, &self->modgrad, &self->logNT,
						   &self->prec, &self->p);
	
    
    for(i=0;i<n;i++)
    {
	  py_region = Py_BuildValue("{s:d, s:d, s:d, s:d, s:d, s:d, s:d}",
						"x1", segs[i*dim], "y1", segs[i*dim+1],
						"x2", segs[i*dim+2], "y2", segs[i*dim+3],
						"width", segs[i*dim+4], "p", segs[i*dim+5],
						"NFA", segs[i*dim+6]);
      PyList_Append(self->regions, py_region);
    }
    

    return 0;
}

static PyMemberDef LSD_members[] = {
    {"regions", T_OBJECT_EX, offsetof(LSD, regions), 0,
     "regions list"},
    {NULL}  /* Sentinel */
};

static PyObject *
LSD_NFA(LSD* self, PyObject *args, PyObject *keywds)
{
	struct rect rec;
	double dx, dy;
	
	
	static char *kwlist[] = {"x1", "y1", "x2",
								"y2", "width", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ddddd", kwlist,
                                     &rec.x1, &rec.y1, &rec.x2,
                                     &rec.y2, &rec.width)) {
		return NULL; 
	}
	// Return to LSD algorithm coordinate system
	if( self->scale != 1.0 )
	  {
		rec.x1 *= self->scale; rec.y1 *= self->scale;
		rec.x2 *= self->scale; rec.y2 *= self->scale;
		rec.width *= self->scale;
	  }
	rec.x1 -= 0.5;
	rec.y1 -= 0.5;
	rec.x2 -= 0.5;
	rec.y2 -= 0.5;
	
	
  
	
	rec.x = (rec.x1 + rec.x2) / 2;
	rec.y = (rec.y1 + rec.y2) / 2;
	
	dx = rec.x2 - rec.x1;
	dy = rec.y2 - rec.y1;
	rec.theta = atan2(dy, dx);
	rec.dx = cos(rec.theta);
	rec.dy = sin(rec.theta);
	rec.prec = self->prec;
	rec.p = self->p;
	double NFA = rect_nfa(&rec, self->angles, self->logNT);
	return Py_BuildValue("d", NFA);
}

static PyObject *
LSD_GRAD(LSD* self, PyObject *args, PyObject *keywds)
{
	double x, y;
	
	static char *kwlist[] = {"x", "y", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "dd", kwlist,
                                     &x, &y)) {
		return NULL; 
	}
	
    // Return to LSD algorithm coordinate system
	if( self->scale != 1.0 )
	  {
		x *= self->scale;
		y *= self->scale;
	  }
	x -= 0.5;
	y -= 0.5;
    
    if (x < 0 || y < 0) {
        return NULL;
    }
    
    if ((int) x > (int) self->angles->xsize || (int) y >= (int) self->angles->ysize) {
        return NULL;
    }
    if ((int) x >= (int) self->modgrad->xsize || (int) y >= (int) self->modgrad->ysize) {
        return NULL;
    }
    
    int angle_ind = (int) (y * self->angles->xsize) + (int) x;
    double angle = self->angles->data[angle_ind];
    int modgrad_ind = (int) (y * self->modgrad->xsize) + (int) x;
    double modgrad = self->modgrad->data[modgrad_ind];
	return Py_BuildValue("[d,d]", modgrad, angle);
}

static PyMethodDef LSD_methods[] = {
    {"NFA", (PyCFunction)LSD_NFA, METH_VARARGS | METH_KEYWORDS,
     "Return -log10(NFA) value for specified region"
    },
    {"grad", (PyCFunction)LSD_GRAD, METH_VARARGS | METH_KEYWORDS,
     "Return gradient vector (module, angle) in given point"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject LSDType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "lsd.lsd",                 /*tp_name*/
    sizeof(LSD),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)LSD_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "LSD objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    LSD_methods,            /* tp_methods */
    LSD_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)LSD_init,      /* tp_init */
    0,                         /* tp_alloc */
    LSD_new,                 /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};



#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initlsd(void)
{
	PyObject* m;

    if (PyType_Ready(&LSDType) < 0)
        return;

    m = Py_InitModule3("lsd", module_methods,
                       "LSD API module.");

    if (m == NULL)
      return;

    Py_INCREF(&LSDType);
    PyModule_AddObject(m, "lsd", (PyObject *)&LSDType);
}
