#include <Elementary.h>
#include "elm_priv.h"

static void
_smart_extents_calculate(Evas_Object *box, Evas_Object_Box_Data *priv, int horizontal, int homogeneous, int extended)
{
	Evas_Coord minw, minh, maxw, maxh, mnw, mnh, ww;
	Evas_Coord w, h, cw = 0, ch = 0, cmaxh = 0, sumw = 0;
	const Eina_List *l;
	Evas_Object_Box_Option *opt;
    double wx;

	/* FIXME: need to calc max */
	minw = 0;
	minh = 0;
	maxw = -1;
	maxh = -1;

	if (homogeneous)
	{
		EINA_LIST_FOREACH(priv->children, l, opt)
		{
			evas_object_size_hint_min_get(opt->obj, &mnw, &mnh);
			if (minh < mnh) minh = mnh;
			if (minw < mnw) minw = mnw;
		}
		if (horizontal)
			minw *= eina_list_count(priv->children);
		else
			minh *= eina_list_count(priv->children);
	}
	else
	{
		if (horizontal && extended)
		{
			evas_object_geometry_get(box, NULL, NULL, &w, &h);
		}

		EINA_LIST_FOREACH(priv->children, l, opt)
		{
			evas_object_size_hint_min_get(opt->obj, &mnw, &mnh);
			evas_object_size_hint_weight_get(opt->obj, &wx, NULL);

			if (horizontal)
			{
				if (extended)
				{
					if(wx)
					{
					   if (mnw != -1 && (w - cw) >= mnw)
						   ww = w - cw;
					   else
						   ww = w;
					}
					else
						ww = mnw;

					if ((cw + mnw) > w)
					{
						minh += cmaxh;
						if (sumw > minw) minw = sumw;

						cw = 0;
						cmaxh = 0;
						sumw = 0;
					}
					cw += ww;
					if (cmaxh < mnh) cmaxh = mnh;

					sumw += mnw;
				}
				else
				{
					if (minh < mnh) minh = mnh;
					minw += mnw;
				}
			}
			else
			{
				if (minw < mnw) minw = mnw;
				minh += mnh;
			}
		}

		if(horizontal && extended)
		{
			minh += cmaxh;
			if (sumw > minw) minw = sumw;
		}

	}
	evas_object_size_hint_min_set(box, minw, minh);
}


static Evas_Coord
_smart_extents_calculate_max_height(Evas_Object *box, Evas_Object_Box_Data *priv, int obj_index)
{
	Evas_Coord mnw, mnh, cw = 0, cmaxh = 0, w, ww;
	const Eina_List *l;
	Evas_Object_Box_Option *opt;
	int index = 0;
    double wx;

	evas_object_geometry_get(box, NULL, NULL, &w, NULL);

	EINA_LIST_FOREACH(priv->children, l, opt)
	{
		evas_object_size_hint_min_get(opt->obj, &mnw, &mnh);
		evas_object_size_hint_weight_get(opt->obj, &wx, NULL);

		if(wx)
		{
			if (mnw != -1 && (w - cw) >= mnw)
				ww = w - cw;
			else
				ww = w;
		}
		else
			ww = mnw;

		if ((cw + ww) > w)
		{
			if (index > obj_index )
			{
				return cmaxh;
			}
			cw = 0;
			cmaxh = 0;
		}

		cw += ww;
		if (cmaxh < mnh) cmaxh = mnh;

		index++;
	}
	
	return cmaxh;
}


void
_els_box_layout(Evas_Object *o, Evas_Object_Box_Data *priv, int horizontal, int homogeneous)
{
	_els_box_layout_ex(o, priv, horizontal, homogeneous, 0);
}

void
_els_box_layout_ex(Evas_Object *o, Evas_Object_Box_Data *priv, int horizontal, int homogeneous, int extended)
{
   Evas_Coord x, y, w, h, xx, yy;
   const Eina_List *l;
   Evas_Object *obj;
   Evas_Coord minw, minh, wdif, hdif;
   int count = 0, expand = 0;
   double ax, ay;
   Evas_Object_Box_Option *opt;

   _smart_extents_calculate(o, priv, horizontal, homogeneous, extended);

   evas_object_geometry_get(o, &x, &y, &w, &h);

   evas_object_size_hint_min_get(o, &minw, &minh);
   evas_object_size_hint_align_get(o, &ax, &ay);
   count = eina_list_count(priv->children);
   if (w < minw)
     {
	x = x + ((w - minw) * (1.0 - ax));
	w = minw;
     }
   if (h < minh)
     {
	y = y + ((h - minh) * (1.0 - ay));
	h = minh;
     }
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        double wx, wy;

	evas_object_size_hint_weight_get(opt->obj, &wx, &wy);
	if (horizontal)
	  {
	     if (wx > 0.0) expand++;
	  }
	else
	  {
	     if (wy > 0.0) expand++;
	  }
     }
   if (expand == 0)
     {
	evas_object_size_hint_align_get(o, &ax, &ay);
	if (horizontal)
	  {
	     x += (double)(w - minw) * ax;
	     w = minw;
	  }
	else
	  {
	     y += (double)(h - minh) * ay;
	     h = minh;
	  }
     }
   wdif = w - minw;
   hdif = h - minh;
   xx = x;
   yy = y;

   Evas_Coord cw = 0, ch = 0, cmaxh = 0, obj_index = 0;

   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Coord mnw, mnh, mxw, mxh;
        double wx, wy;
        int fw, fh, xw, xh;

	obj = opt->obj;
	evas_object_size_hint_align_get(obj, &ax, &ay);
	evas_object_size_hint_weight_get(obj, &wx, &wy);
	evas_object_size_hint_min_get(obj, &mnw, &mnh);
	evas_object_size_hint_max_get(obj, &mxw, &mxh);
	fw = fh = 0;
	xw = xh = 0;
	if (ax == -1.0) {fw = 1; ax = 0.5;}
	if (ay == -1.0) {fh = 1; ay = 0.5;}
	if (wx > 0.0) xw = 1;
	if (wy > 0.0) xh = 1;
	if (horizontal)
	  {
	     if (homogeneous)
	       {
		  Evas_Coord ww, hh, ow, oh;

		  ww = (w / (Evas_Coord)count);
		  hh = h;
		  ow = mnw;
		  if (fw) ow = ww;
		  if ((mxw >= 0) && (mxw < ow))
		    ow = mxw;
		  oh = mnh;
		  if (fh) oh = hh;
		  if ((mxh >= 0) && (mxh < oh))
		    oh = mxh;
		  evas_object_move(obj,
				   xx + (Evas_Coord)(((double)(ww - ow)) * ax),
				   yy + (Evas_Coord)(((double)(hh - oh)) * ay));
		  evas_object_resize(obj, ow, oh);
		  xx += ww;
	       }
	     else
	       {
			   if (extended)
			   {
				   Evas_Coord ww, hh, ow, oh;
				   if(wx)
				   {
					   if (mnw != -1 && (w - cw) >= mnw)
						   ww = w - cw;
					   else
						   ww = w;
				   }
				   else
					   ww = mnw;
				   hh = _smart_extents_calculate_max_height(o, priv, obj_index);

				   ow = mnw;
				   if (fw) ow = ww;
				   if ((mxw >= 0) && (mxw < ow)) ow = mxw;
				   oh = mnh;
				   if (fh) oh = hh;
				   if ((mxh >= 0) && (mxh < oh)) oh = mxh;

				   if ((cw + ww) > w)
				   {
					   ch += cmaxh;

					   cw = 0;
					   cmaxh = 0;
				   }

				   evas_object_move(obj,
						   xx + cw + (Evas_Coord)(((double)(ww - ow)) * ax),
						   yy + ch + (Evas_Coord)(((double)(hh - oh)) * ay));
				   evas_object_resize(obj, ow, oh);

				   cw += ww;
				   if (cmaxh < hh) cmaxh = hh;
			   }
			   else
			   {
				  Evas_Coord ww, hh, ow, oh;

				  ww = mnw;
				  if ((expand > 0) && (xw))
					{
					   if (expand == 1) ow = wdif;
					   else ow = (w - minw) / expand;
					   wdif -= ow;
					   ww += ow;
					}
				  hh = h;
				  ow = mnw;
				  if (fw) ow = ww;
				  if ((mxw >= 0) && (mxw < ow)) ow = mxw;
				  oh = mnh;
				  if (fh) oh = hh;
				  if ((mxh >= 0) && (mxh < oh)) oh = mxh;
				  evas_object_move(obj,
						   xx + (Evas_Coord)(((double)(ww - ow)) * ax),
						   yy + (Evas_Coord)(((double)(hh - oh)) * ay));
				  evas_object_resize(obj, ow, oh);
				  xx += ww;
			   }
	       }
	  }
	else
	  {
	     if (homogeneous)
	       {
		  Evas_Coord ww, hh, ow, oh;

		  ww = w;
		  hh = (h / (Evas_Coord)count);
		  ow = mnw;
		  if (fw) ow = ww;
		  if ((mxw >= 0) && (mxw < ow)) ow = mxw;
		  oh = mnh;
		  if (fh) oh = hh;
		  if ((mxh >= 0) && (mxh < oh)) oh = mxh;
		  evas_object_move(obj,
				   xx + (Evas_Coord)(((double)(ww - ow)) * ax),
				   yy + (Evas_Coord)(((double)(hh - oh)) * ay));
		  evas_object_resize(obj, ow, oh);
		  yy += hh;
	       }
	     else
	       {
		  Evas_Coord ww, hh, ow, oh;

		  ww = w;
		  hh = mnh;
		  if ((expand > 0) && (xh))
		    {
		       if (expand == 1) oh = hdif;
		       else oh = (h - minh) / expand;
		       hdif -= oh;
		       hh += oh;
		    }
		  ow = mnw;
		  if (fw) ow = ww;
		  if ((mxw >= 0) && (mxw < ow)) ow = mxw;
		  oh = mnh;
		  if (fh) oh = hh;
		  if ((mxh >= 0) && (mxh < oh)) oh = mxh;
		  evas_object_move(obj,
				   xx + (Evas_Coord)(((double)(ww - ow)) * ax),
				   yy + (Evas_Coord)(((double)(hh - oh)) * ay));
		  evas_object_resize(obj, ow, oh);
		  yy += hh;
	       }
	  }

		obj_index++;
     }
}

