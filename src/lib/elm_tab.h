/*
 * SLP
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics. 
 */

/**
 * library for composing TW3 tab
 * @file tab_ui.h
 * @ingroup library libtab
 * @brief this file is the header file including UI macros
 * @author Jaehwan Kim <jae.hwan.kim@samsung.com>
 * @date 2009-08-07
 */

#ifndef __TAB_UI_H__
#define __TAB_UI_H__


/////////////////////////////////////////////////////////////////////////////
//
//  UTIL define
//
/////////////////////////////////////////////////////////////////////////////
#define MAX_ITEM 8
#define BASIC_SLOT_NUMBER 3
#define ELM_MAX(v1, v2)    (((v1) > (v2)) ? (v1) : (v2))
#define KEYDOWN_INTERVAL	0.6

#define _EDJ(x) (Evas_Object *)elm_layout_edje_get(x)

#endif
