/*-----------------------------------------------------------------------------
 *
 * World Magnetic Model - C python bindings - magnetic model evaluation
 *
 * Project: World Magnetic Model - python interface
 * Author: Martin Paces <martin.paces@eox.at>
 *
 *
 *-----------------------------------------------------------------------------
 * Copyright (C) 2014 EOX IT Services GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *-----------------------------------------------------------------------------
*/

#ifndef PYWMM_GEOMAG_H
#define PYWMM_GEOMAG_H

#include <math.h>
#include <string.h>
#include <stdlib.h>

//#include "GeomagnetismHeader.h"

#include "math_aux.h"
#include "geo_conv.h"
#include "shc.h"
#include "pywmm_aux.h"
#include "pywmm_coord.h"
#include "pywmm_cconv.h"

#ifndef NAN
#define NAN (0.0/0.0)
#endif

/* Earth radius in km */
#ifndef RADIUS
#define RADIUS  6371.2
#endif

#ifndef STR
#define SRINGIFY(x) #x
#define STR(x) SRINGIFY(x)
#endif

/* mode of evaluation enumerate */
typedef enum {
    GM_INVALID = 0x0,
    GM_POTENTIAL = 0x1,
    GM_GRADIENT = 0x2,
    GM_POTENTIAL_AND_GRADIENT = 0x3,
} GEOMAG_MODE;

/* mode of evaluation check */
static GEOMAG_MODE _check_geomag_mode(int mode, const char *label)
{
    switch (mode)
    {
        case GM_POTENTIAL:
            return GM_POTENTIAL;
        case GM_GRADIENT:
            return GM_GRADIENT;
        case GM_POTENTIAL_AND_GRADIENT:
            return GM_POTENTIAL_AND_GRADIENT;
        default:
            PyErr_Format(PyExc_ValueError, "Invalid mode '%s'!", label);
            return GM_INVALID;
    }
}

/* magnetic model - auxiliary structure */
typedef struct {
    int degree;
    int nterm;
    int coord_in;
    int coord_out;
    double elps_a;
    double elps_eps2;
    double crad_ref;
    double clat_last;
    double clon_last;
    double crad_last;
    const double *cg;
    const double *ch;
    double *lp;
    double *ldp;
    double *lsin;
    double *lcos;
    double *rrp;
    double *psqrt;
} MODEL;

/* model stucture destruction */
static void _geomag_model_destroy(MODEL *model)
{
    if(NULL != model->lp)
        free(model->lp);
    if(NULL != model->ldp)
        free(model->ldp);
    if(NULL != model->lsin)
        free(model->lsin);
    if(NULL != model->lcos)
        free(model->lcos);
    if(NULL != model->rrp)
        free(model->rrp);
    if(NULL != model->psqrt)
        free(model->psqrt);

    memset(model, 0, sizeof(MODEL));
}

/* model stucture initiliazation */
static int _geomag_model_init(MODEL *model, int degree,
    int coord_in, int coord_out, const double *cg, const double *ch)
{
    const int nterm = ((degree+1)*(degree+2))/2;

    memset(model, 0, sizeof(MODEL));

    model->degree = degree;
    model->nterm = nterm;
    model->coord_in = coord_in;
    model->coord_out = coord_out;
    model->elps_a = WGS84_A;
    model->elps_eps2 = WGS84_EPS2;
    model->crad_ref = RADIUS;
    model->clat_last = NAN;
    model->clon_last = NAN;
    model->crad_last = NAN;
    model->cg = cg;
    model->ch = ch;

    if (NULL == (model->lp = (double*)malloc(nterm*sizeof(double))))
        goto error;

    if (NULL == (model->ldp = (double*)malloc(nterm*sizeof(double))))
        goto error;

    if (NULL == (model->lsin = (double*)malloc((degree+1)*sizeof(double))))
        goto error;

    if (NULL == (model->lcos = (double*)malloc((degree+1)*sizeof(double))))
        goto error;

    if (NULL == (model->rrp = (double*)malloc((degree+1)*sizeof(double))))
        goto error;

    if (NULL == (model->psqrt = shc_presqrt(degree)))
        goto error;

    return 0;

  error:
    _geomag_model_destroy(model);
    return 1;
}

/* single point evaluation */
static void _geomag_model_eval(MODEL *model, int mode, double *fpot,
    double *fx, double *fy, double *fz, double x, double y, double z)
{
    double glat, glon, ghgt;
    double clat, clon, crad;
    double flat, flon, frad;
    double tmp;

    // convert the input coordinates
    switch(model->coord_in)
    {
        case CT_GEODETIC_ABOVE_WGS84:
            glat = x; glon = y; ghgt = z;
            geodetic2geocentric_sph(&crad, &clat, &clon, glat, glon, ghgt, model->elps_a, model->elps_eps2);
            break;
        case CT_GEODETIC_ABOVE_EGM96:
            glat = x; glon = y; ghgt = z + delta_EGM96_to_WGS84(glat, glon);
            geodetic2geocentric_sph(&crad, &clat, &clon, glat, glon, ghgt, model->elps_a, model->elps_eps2);
            break;
        case CT_GEOCENTRIC_SPHERICAL:
            clat = DG2RAD*x; clon = DG2RAD*y; crad = z;
            geocentric_sph2geodetic(&glat, &glon, &ghgt, crad, clat, clon, model->elps_a, model->elps_eps2);
            break;
        case CT_GEOCENTRIC_CARTESIAN:
            cart2sph(&crad, &clat, &clon, x, y, z);
            geocentric_sph2geodetic(&glat, &glon, &ghgt, crad, clat, clon, model->elps_a, model->elps_eps2);
        default:
            return;
    }

    // associative Legendre functions
    if (model->clat_last != clat)
        shc_legendre(model->lp, model->ldp, model->degree, clat, model->psqrt);

    // relative radial powers
    if (model->clon_last != clon)
        shc_azmsincos(model->lsin, model->lcos, model->degree, clon);

    // sines/cosines of the longitude
    if (model->crad_last != crad)
        shc_relradpow(model->rrp, model->degree, crad/model->crad_ref);

    // save the last evaluated coordinate
    model->clat_last = clat;
    model->clon_last = clon;
    model->crad_last = crad;

    // evaluate the model
    shc_eval(fpot, &flat, &flon, &frad, model->degree, mode,
                clat, crad, model->cg, model->ch, model->lp, model->ldp,
                model->rrp, model->lsin, model->lcos);

    // project the produced vectors to the desired coordinate system
    if (mode&0x2)
    {
        switch(model->coord_out)
        {
            case CT_GEOCENTRIC_SPHERICAL:
                *fx = flat;
                *fy = flon;
                *fz = frad;
                break;

            case CT_GEODETIC_ABOVE_WGS84:
            case CT_GEODETIC_ABOVE_EGM96:
                tmp = DG2RAD*glat - clat;
                rot2d(fz, fx, frad, flat, sin(tmp), cos(tmp));
                *fy = flon;
                break;

            case CT_GEOCENTRIC_CARTESIAN:
                rot2d(&tmp, fz, frad, flat, sin(clat), cos(clat));
                rot2d(fx, fy, tmp, flon, sin(clon), cos(clon));
                break;
        }
    }
}

/* recursive batch model_evaluation for the input numpy arrays of coordinates */
#define S(a) ((double*)(a).data)
#define P(a,i) ((double*)((a).data+(i)*(a).stride[0]))
#define V(a,i) (*P(a,i))

static void _geomag1(ARRAY_DATA arrd_in, ARRAY_DATA arrd_pot, MODEL *model)
{
    if (arrd_in.ndim > 1)
    {
        npy_intp i;
        for(i = 0; i < arrd_in.dim[0]; ++i)
            _geomag1(_get_arrd_item(&arrd_in, i), _get_arrd_item(&arrd_pot, i), model);
        return;
    }

    _geomag_model_eval(model, GM_POTENTIAL, S(arrd_pot), NULL, NULL, NULL,
                    V(arrd_in, 0), V(arrd_in, 1), V(arrd_in, 2));
}

static void _geomag2(ARRAY_DATA arrd_in, ARRAY_DATA arrd_grd, MODEL *model)
{
    if (arrd_in.ndim > 1)
    {
        npy_intp i;
        for(i = 0; i < arrd_in.dim[0]; ++i)
            _geomag2(_get_arrd_item(&arrd_in, i), _get_arrd_item(&arrd_grd, i), model);
        return;
    }

    _geomag_model_eval(model, GM_GRADIENT, NULL, P(arrd_grd, 0), P(arrd_grd, 1),
        P(arrd_grd, 2), V(arrd_in, 0), V(arrd_in, 1), V(arrd_in, 2));

}

static void _geomag3(ARRAY_DATA arrd_in, ARRAY_DATA arrd_pot, ARRAY_DATA arrd_grd, MODEL *model)
{
    if (arrd_in.ndim > 1)
    {
        npy_intp i;
        for(i = 0; i < arrd_in.dim[0]; ++i)
            _geomag3(_get_arrd_item(&arrd_in, i), _get_arrd_item(&arrd_pot, i),
                    _get_arrd_item(&arrd_grd, i), model);
        return;
    }

    _geomag_model_eval(model, GM_POTENTIAL_AND_GRADIENT, S(arrd_pot),
        P(arrd_grd, 0), P(arrd_grd, 1), P(arrd_grd, 2), V(arrd_in, 0),
        V(arrd_in, 1), V(arrd_in, 2));
}

#undef V
#undef P
#undef S

/* python function definition */

#define DOC_GEOMAG "\n"\
"   arr_out = geomag_static(arr_in, degree, coef_g, coef_h, coord_type_in=GEODETIC_ABOVE_WGS84,\n"\
"                          coord_type_out=GEODETIC_ABOVE_WGS84, mode=GM_GRADIENT)"\
"     Parameters:\n"\
"       arr_in - array of 3D coordinates (up to 16 dimensions).\n"\
"       degree - degree of the spherical harmonic model.\n"\
"       coef_g - vector of spherical harmonic model coeficients.\n"\
"       coef_h - vector of spherical harmonic model coeficients.\n"\
"       coord_type_in - type of the input coordinates.\n"\
"       mode - quantity to be evaluated:\n"\
"                  POTENTIAL\n"\
"                  GRADIENT (default)\n"\
"                  POTENTIAL_AND_GRADIENT\n"\
"       rad_ref - reference (Earth) radius\n"


static PyObject* geomag(PyObject *self, PyObject *args, PyObject *kwdict)
{
    static char *keywords[] = {"arr_in", "degree", "coef_g", "coef_h",
                 "coord_type_in", "coord_type_out", "mode", NULL};
    int ct_in = CT_GEODETIC_ABOVE_WGS84;
    int ct_out = CT_GEODETIC_ABOVE_WGS84;
    int nterm, degree = 0, mode = 0x2;
    PyObject *obj_in = NULL; // input object
    PyObject *obj_cg = NULL; // coef_g object
    PyObject *obj_ch = NULL; // coef_h object
    PyObject *arr_in = NULL; // input array
    PyObject *arr_cg = NULL; // coef_g array
    PyObject *arr_ch = NULL; // coef_h array
    PyObject *arr_pot = NULL; // output array
    PyObject *arr_grd = NULL; // output array
    PyObject *retval = NULL;
    MODEL model;

    // parse input arguments
    if (!PyArg_ParseTupleAndKeywords(args, kwdict,
            "OiOO|iii:geomag", keywords, &obj_in, &degree, &obj_cg, &obj_ch,
            &ct_in, &ct_out, &mode))
        goto exit;

    // check the type of the coordinate transformation
    if (CT_INVALID == _check_coord_type(ct_in, keywords[4]))
        goto exit;

    if (CT_INVALID == _check_coord_type(ct_out, keywords[5]))
        goto exit;

    // check the operation mode
    if (GM_INVALID == _check_geomag_mode(mode, keywords[6]))
        goto exit;

    // cast the objects to arrays
    if (NULL == (arr_in=_get_as_double_array(obj_in, 1, 0, NPY_ALIGNED, keywords[0])))
        goto exit;

    if (NULL == (arr_cg=_get_as_double_array(obj_cg, 1, 1, NPY_IN_ARRAY, keywords[2])))
        goto exit;

    if (NULL == (arr_ch=_get_as_double_array(obj_ch, 1, 1, NPY_IN_ARRAY, keywords[3])))
        goto exit;

    if (degree < 1)
    {
        PyErr_Format(PyExc_ValueError, "Invalid value %d of '%s'!", degree, keywords[1]);
        goto exit;
    }

    // check dimensions of the coeficient arrays
    nterm = ((degree+1)*(degree+2))/2;

    // check maximum allowed input array dimension
    if (PyArray_NDIM(arr_in) > MAX_OUT_ARRAY_NDIM)
    {
        PyErr_Format(PyExc_ValueError, "The input array dimension of '%s'"\
            " %d exceeds the allowed maximum value %d!", keywords[0],
            PyArray_NDIM(arr_in), MAX_OUT_ARRAY_NDIM);
        goto exit;
    }

    // check the last dimension (required length of the coordinates vectors)
    if (_check_array_dim_eq(arr_in, -1, 3, keywords[0]))
        goto exit;

    // check the coeficients arrays' dimensions
    if (_check_array_dim_le(arr_cg, 0, nterm, keywords[2]))
        goto exit;

    if (_check_array_dim_le(arr_ch, 0, nterm, keywords[3]))
        goto exit;

    // create the output arrays
    if (mode & GM_POTENTIAL)
        if (NULL == (arr_pot = PyArray_EMPTY(PyArray_NDIM(arr_in)-1, PyArray_DIMS(arr_in), NPY_DOUBLE, 0)))
            goto exit;

    if (mode & GM_GRADIENT)
        if (NULL == (arr_grd = _get_new_double_array(PyArray_NDIM(arr_in), PyArray_DIMS(arr_in), 3)))
            goto exit;

    // evaluate the model

    if(_geomag_model_init(&model, degree, ct_in, ct_out, PyArray_DATA(arr_cg), PyArray_DATA(arr_ch)))
        goto exit;

    switch(mode)
    {
        case GM_POTENTIAL:
            _geomag1(_array_to_arrd(arr_in), _array_to_arrd(arr_pot), &model);
            retval = arr_pot;
            break;

        case GM_GRADIENT:
            _geomag2(_array_to_arrd(arr_in), _array_to_arrd(arr_grd), &model);
            retval = arr_grd;
            break;

        case GM_POTENTIAL_AND_GRADIENT:
            _geomag3(_array_to_arrd(arr_in), _array_to_arrd(arr_pot), _array_to_arrd(arr_grd), &model);
            if (NULL == (retval = Py_BuildValue("NN", arr_pot, arr_grd)))
                goto exit;
            break;
    }

    _geomag_model_destroy(&model);

  exit:

    // decrease reference counters to the arrays
    if (arr_in){Py_DECREF(arr_in);}
    if (arr_cg){Py_DECREF(arr_cg);}
    if (arr_ch){Py_DECREF(arr_ch);}
    if (!retval && arr_grd){Py_DECREF(arr_grd);}
    if (!retval && arr_pot){Py_DECREF(arr_pot);}

    return retval;
}

#endif  /* PYWMM_GEOMAG_H */
