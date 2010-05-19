/*
 * SLP
 * Copyright (c) 2010 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */


/**
 *
 * @addtogroup Animator
 *
 * Animator 
*/

#include <Elementary.h>



#define ELM_FX_EXCEPTION_ENABLE        

#define ELM_FX_NULL_CHECK( obj ) \
	             if( ( obj ) == 0 ) \
                            return  

#define ELM_FX_NULL_CHECK_WITH_RET( obj, ret ) \
		   if( ( obj ) == 0 ) \
			    return ret


static const float _in_out_table[181] = { 
0, 3.08652e-05, 0.000123472, 0.000277855, 0.000494071, 0.000772201, 0.00111235, 0.00151464, 0.00197923, 0.00250628, 0.00309601, 0.00374862, 0.00446438, 0.00524354, 0.00608643, 0.00699335, 0.00796467,
0.00900077, 0.0101021, 0.011269, 0.012502, 0.0138016, 0.0151684, 0.0166028, 0.0181056, 0.0196773, 0.0213187, 0.0230304, 0.0248132, 0.026668, 0.0285955, 0.0305966, 0.0326724, 0.0348238, 0.0370519,
0.0393577, 0.0417424, 0.0442074, 0.0467538, 0.0493831, 0.0520968, 0.0548963, 0.0577834, 0.0607596, 0.063827, 0.0669873, 0.0702427, 0.0735953, 0.0770474, 0.0806015, 0.0842603, 0.0880264, 0.0919029,
0.095893, 0.1, 0.104228, 0.10858, 0.11306, 0.117674,  0.122427, 0.127322,  0.132367, 0.137567, 0.142929, 0.14846, 0.154169, 0.160065, 0.166158, 0.172458, 0.178977,0.18573, 0.192732, 0.2, 0.207554,
0.215417, 0.223615, 0.232178, 0.241144, 0.250556, 0.260466, 0.270939, 0.282055, 0.29392, 0.30667, 0.320495, 0.335664, 0.352594, 0.371981, 0.395178, 0.425672, 0.5, 0.574328, 0.604822, 0.628019,
0.647406, 0.664336, 0.679505, 0.69333, 0.70608, 0.717945, 0.729061, 0.739534, 0.749444, 0.758856, 0.767822, 0.776385, 0.784583, 0.792446, 0.8, 0.807268, 0.81427, 0.821023, 0.827542, 0.833842,
0.839935, 0.845831, 0.85154, 0.857071, 0.862433, 0.867633, 0.872678, 0.877573, 0.882326, 0.88694, 0.89142, 0.895772, 0.9, 0.904107, 0.908097, 0.911974, 0.91574, 0.919398, 0.922953, 0.926405,
0.929757, 0.933013, 0.936173, 0.93924, 0.942217, 0.945104, 0.947903, 0.950617, 0.953246, 0.955793, 0.958258, 0.960642, 0.962948, 0.965176, 0.967328, 0.969403, 0.971405, 0.973332, 0.975187, 0.97697,
0.978681, 0.980323, 0.981894, 0.983397, 0.984832, 0.986198, 0.987498, 0.988731, 0.989898, 0.990999, 0.992035, 0.993007, 0.993914, 0.994756, 0.995536, 0.996251, 0.996904, 0.997494, 0.998021, 0.998485,
0.998888, 0.999228, 0.999506, 0.999722, 0.999877, 0.999969, 1 }; 


static const float _in_table[181] = { 
0, 1.54322e-05, 6.17303e-05, 0.000138899, 0.000246944, 0.000385877, 0.00055571, 0.000756459, 0.000988143, 0.00125078, 0.0015444, 0.00186903, 0.0022247, 0.00261143, 0.00302928, 0.00347827, 0.00395845,
0.00446987, 0.00501256, 0.00558659, 0.00619201, 0.00682887, 0.00749724, 0.00819718, 0.00892875, 0.00969203, 0.0104871, 0.011314, 0.0121729, 0.0130637, 0.0139867, 0.0149419, 0.0159293, 0.0169492,
0.0180015, 0.0190865, 0.0202041, 0.0213546, 0.0225379, 0.0237544, 0.025004, 0.0262869, 0.0276032, 0.0289531, 0.0303367, 0.0317542, 0.0332056, 0.0346912, 0.0362112, 0.0377656, 0.0393546, 0.0409785,
0.0426374, 0.0443314, 0.0460608, 0.0478257, 0.0496265, 0.0514631, 0.0533359, 0.0552451, 0.057191, 0.0591736, 0.0611933, 0.0632503, 0.0653449, 0.0674772, 0.0696476, 0.0718564, 0.0741037, 0.0763899,
0.0787153, 0.0810802, 0.0834849, 0.0859296, 0.0884148, 0.0909407, 0.0935076, 0.0961161, 0.0987663, 0.101459, 0.104194, 0.106971, 0.109793, 0.112658, 0.115567, 0.11852, 0.121519, 0.124564,  0.127654,
0.130791, 0.133975, 0.137206, 0.140485, 0.143813, 0.147191, 0.150618, 0.154095, 0.157623, 0.161203, 0.164835, 0.168521, 0.17226, 0.176053, 0.179901, 0.183806, 0.187767, 0.191786, 0.195863, 0.2,
0.204197, 0.208455, 0.212776, 0.217159, 0.221607, 0.226121, 0.230701, 0.235349, 0.240066, 0.244853, 0.249712, 0.254644, 0.259651, 0.264733, 0.269893, 0.275133, 0.280454, 0.285857, 0.291345, 0.29692,
0.302584, 0.308339, 0.314187, 0.320131, 0.326173, 0.332316, 0.338562, 0.344915, 0.351378, 0.357955, 0.364647, 0.371461, 0.378398, 0.385464, 0.392663, 0.4, 0.40748, 0.415108, 0.42289, 0.430833,
0.438944, 0.447229, 0.455697, 0.464357, 0.473217, 0.482289, 0.491583, 0.501112, 0.51089, 0.520932, 0.531255, 0.541877, 0.552821, 0.56411, 0.575772, 0.587839, 0.600347, 0.61334, 0.626867, 0.640989,
0.65578, 0.671329, 0.68775, 0.705189, 0.723838, 0.743962, 0.76594, 0.790356, 0.818188, 0.851343, 0.894737, 1 }; 


static const float _out_table[181] = { 
0, 0.105263, 0.148657, 0.181812, 0.209644, 0.23406, 0.256038, 0.276162, 0.294811, 0.31225, 0.328671, 0.34422, 0.359011, 0.373133, 0.38666, 0.399653, 0.412161, 0.424228, 0.43589, 0.447179, 0.458123,
0.468745, 0.479068, 0.48911, 0.498888, 0.508417, 0.517711, 0.526783, 0.535643, 0.544303, 0.552771, 0.561056, 0.569167, 0.57711, 0.584892, 0.59252, 0.6, 0.607337, 0.614536, 0.621602, 0.628539, 0.635353,
0.642045, 0.648622, 0.655085, 0.661438, 0.667684, 0.673827, 0.679869, 0.685813, 0.691661, 0.697416, 0.70308, 0.708655, 0.714143, 0.719546, 0.724867, 0.730107, 0.735267, 0.740349, 0.745356, 0.750288,
0.755147, 0.759934, 0.764651, 0.769299, 0.773879, 0.778393, 0.782841, 0.787224, 0.791545, 0.795803, 0.8, 0.804137, 0.808214, 0.812233, 0.816194, 0.820099, 0.823947, 0.82774, 0.831479, 0.835165,
0.838797, 0.842377, 0.845905, 0.849382, 0.852809, 0.856187, 0.859515, 0.862794, 0.866025, 0.869209, 0.872346, 0.875436, 0.878481, 0.88148, 0.884433, 0.887342, 0.890207, 0.893029, 0.895806, 0.898541,
0.901234, 0.903884, 0.906492, 0.909059, 0.911585, 0.91407, 0.916515, 0.91892, 0.921285, 0.92361, 0.925896, 0.928144, 0.930352, 0.932523, 0.934655, 0.93675, 0.938807, 0.940826, 0.942809, 0.944755,
0.946664, 0.948537, 0.950374, 0.952174, 0.953939, 0.955669, 0.957363, 0.959021, 0.960645, 0.962234, 0.963789, 0.965309, 0.966794, 0.968246, 0.969663, 0.971047, 0.972397, 0.973713, 0.974996, 0.976246,
0.977462, 0.978645, 0.979796, 0.980914, 0.981998, 0.983051, 0.984071, 0.985058, 0.986013, 0.986936, 0.987827,0.988686, 0.989513, 0.990308, 0.991071, 0.991803, 0.992503, 0.993171, 0.993808, 0.994413,
0.994987, 0.99553, 0.996042, 0.996522, 0.996971, 0.997389, 0.997775, 0.998131, 0.998456,  0.998749, 0.999012, 0.999244, 0.999444, 0.999614, 0.999753, 0.999861, 0.999938, 0.999985, 1 }; 


struct _animator {

	Evas_Object* 	 	parent;
	Ecore_Timer* 		timer;

	double 			begin_time;
	double          	cur_time;
	double          	duration;
	unsigned int   		repeat_cnt;
	unsigned int		cur_repeat_cnt;	

	double 			(*curve_op)( const double );

	void           		(*animator_op)(void*, Elm_Animator*, const double);
	void*           	animator_arg;    

	void   			(*completion_op)(void*);  
	void*           	completion_arg;

	Eina_Bool       	auto_reverse :1;
	Eina_Bool       	on_animating : 1;
		
};


inline static double _animator_curve_linear( const double frame );
inline static double _animator_curve_in_out( const double frame );
inline static double _animator_curve_in( const double frame );
inline static double _animator_curve_out( const double frame );
inline static unsigned int _animator_compute_reverse_repeat_count( unsigned int cnt );
inline static unsigned int _animator_compute_no_reverse_repeat_count( unsigned int cnt );
static int _animator_animate_cb( void* data );
inline static void _delete_timer( Elm_Animator* animator );


inline static unsigned int _animator_compute_reverse_repeat_count( unsigned int cnt ) 
{
	return ( ( cnt  + 1 ) << 1 ) - 1;
}

inline static unsigned int _animator_compute_no_reverse_repeat_count( unsigned int cnt )
{
	return cnt >> 1;
}

inline static double _animator_curve_linear( const double frame )
{
	return frame;
}


inline static double _animator_curve_in_out( const double frame )
{
	return _in_out_table[ (int) ( frame * 180 ) ];
}

inline static double _animator_curve_in( const double frame )
{
	return  _in_table[ (int) ( frame * 180 ) ];
}

inline static double _animator_curve_out( const double frame )
{
	return  _out_table[ (int) ( frame * 180 ) ];

}

inline static void _delete_timer( Elm_Animator* animator )
{
	if( animator->timer ) {
		ecore_timer_del( animator->timer );
		animator->timer = NULL;
	}
}




static int _animator_animate_cb( void* data ) 
{
	Elm_Animator* animator = (Elm_Animator*) data;		

	animator->cur_time = ecore_time_get(); 

	double elapsed_time = animator->cur_time - animator->begin_time;

	//TODO: HOW TO MAKE IT PRECIOUS TIME? -> Use Interpolation!!
	if( elapsed_time > animator->duration ) {
		elapsed_time = animator->duration;
	}

	float frame = animator->curve_op( elapsed_time / animator->duration );

	//Reverse?
	if( animator->auto_reverse == EINA_TRUE ) {
		if( animator->cur_repeat_cnt % 2 == 0 ) {
			frame = 1 - frame;
		}
	}

	if( animator->duration > 0 ) {
		animator->animator_op( animator->animator_arg, 
				       animator,  
				       frame );
	}

	//Not end. Keep going.
	if( elapsed_time < animator->duration ) {
		return ECORE_CALLBACK_RENEW;
	}

	//Repeat and reverse and time done! 
	if( animator->cur_repeat_cnt == 0 ) {

		animator->on_animating = EINA_FALSE;
		_delete_timer( animator );
		animator->completion_op( animator->completion_arg );

		return ECORE_CALLBACK_CANCEL;	
	}
	
	//Repeat Case
	--animator->cur_repeat_cnt;
	animator->begin_time = ecore_time_get();

	return ECORE_CALLBACK_RENEW;

}

/**
 * @ingroup Animator 
 *
 * Get the value of reverse mode. 
 *
 * @param  animator 	 Animator object
 * @return 		 EINA_TRUE is reverse mode 
 */
inline EAPI Eina_Bool elm_animator_auto_reverse_get( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE_WITH_RET
	ELM_FX_NULL_CHECK( animator, EINA_FALSE );
#endif

	return animator->auto_reverse;
}



/**
 * @ingroup Animator 
 *
 * Get the value of repeat count.
 *
 * @param  animator 	 Animator object
 * @return 		 Repeat count
 */
inline EAPI unsigned int elm_animator_repeat_get( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE_WITH_RET
	ELM_FX_NULL_CHECK( animator, EINA_FALSE );
#endif

	return animator->repeat_cnt;
}



/**
 * @ingroup Animator 
 *
 * Set auto reverse function.  
 *
 * @param  animator 	 Animator object
 * @param  reverse 	 Reverse or not
 */
EAPI void elm_animator_auto_reverse_set( Elm_Animator* animator, Eina_Bool reverse )
{
#ifdef ELM_FX_EXCEPTION_ENABLE
	ELM_FX_NULL_CHECK( animator );
#endif
	if( animator->auto_reverse == reverse ) {
		return;
	}

	animator->auto_reverse = reverse;

	if( reverse == EINA_TRUE ) {
		animator->repeat_cnt = _animator_compute_reverse_repeat_count( animator->repeat_cnt );
	}else {
		animator->repeat_cnt = _animator_compute_no_reverse_repeat_count( animator->repeat_cnt ); 
	}
}



/**
 * @ingroup Animator
 *
 * Set the animation acceleration style. 
 *
 * @param  animator     Animator object
 * @param  cs           Curve style. Default is ELM_ANIMATOR_CURVE_LINEAR 
 */
EAPI void elm_animator_curve_style_set( Elm_Animator* animator, Elm_Animator_Curve_Style cs )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );
#endif 

	switch( cs ) {

		case ELM_ANIMATOR_CURVE_LINEAR:
			animator->curve_op = _animator_curve_linear;
			break;
		case ELM_ANIMATOR_CURVE_IN_OUT:
			animator->curve_op = _animator_curve_in_out;
			break;
		case ELM_ANIMATOR_CURVE_IN:
			animator->curve_op = _animator_curve_in;
			break;
		case ELM_ANIMATOR_CURVE_OUT:
			animator->curve_op = _animator_curve_out;
			break;
		default:
			fprintf( stderr, "What Animation Curve Style?!\n" );
			animator->curve_op = _animator_curve_linear;
			break;

	}
}


/**
 * @ingroup Animator
 *
 * Set the operation duration.  
 *
 * @param  animator     Animator object
 * @param  duration     Duration in second 
 */
inline EAPI void elm_animator_duration_set( Elm_Animator* animator, const double duration ) 
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );
#endif 

	if( animator->on_animating == EINA_TRUE ) {
		return ;
	}
	
	animator->duration = duration;

#ifdef ELM_FX_EXCEPTION_ENABLE
	if( animator->duration < 0 ) {
		animator->duration = 0;
	}
#endif

}




/**
 * @ingroup Animator
 *
 * Set the callback function for animator operation.  
 *
 * @param     animator 		Animator object
 * @param     op      		Callback function pointer 
 * @param     data              Callback function user argument 
 */
inline EAPI void elm_animator_operation_callback_set( Elm_Animator* animator, void (*op)(void*, Elm_Animator*, const double), void* data ) 
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );
#endif 
	if( animator->on_animating == EINA_TRUE ) {
		return ;
	}
	
	animator->animator_op = op;
	animator->animator_arg = data;
}






/**
 * @ingroup Animator
 *
 * Set the callback function for animator operation. (deprecated. Try use elm_animator_operation_callback_set) 
 *
 * @param     animator 		Animator object
 * @param     op      		Callback function pointer 
 * @param     data              Callback function user argument 
 */
inline EAPI void elm_animator_operation_set( Elm_Animator* animator, void (*op)(void*, Elm_Animator*, const double), void* data ) 
{
	fprintf( stderr, "elm_animator_operation_set is deprecated!, Try use elm_animator_operation_callback_set!\n" );

	elm_animator_operation_callback_set( animator, op, data );
}





/**
 * @ingroup Animator
 *
 * Add new animator. 
 *
 * @param    parent     Parent object
 * @return   		Animator object 
 */
EAPI Elm_Animator* elm_animator_add( Evas_Object* parent )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK_WITH_RET( parent, NULL );
#endif
	
	Elm_Animator* animator = calloc( 1, sizeof( Elm_Animator ) );

	if( animator == NULL ) {
		fprintf( stderr, "Failed to allocate Elm_Animator!\n" );
		return NULL;
	}

	animator->parent = parent;
	elm_animator_auto_reverse_set( animator, EINA_FALSE );
	elm_animator_curve_style_set( animator, ELM_ANIMATOR_CURVE_LINEAR );
	
	return animator;
}




/**
 * @ingroup Animator
 *
 * Get the status for the animator operation.
 *
 * @param    animator   Animator object 
 * @return              EINA_TRUE is animator is operating. 
 */
EAPI Eina_Bool elm_animator_operating_get( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK_WITH_RET( animator, EINA_FALSE );
#endif
	return animator->on_animating; 
}




/**
 * @ingroup Animator
 *
 * Delete animator. 
 *
 * @param    animator   Animator object 
 */
EAPI void elm_animator_del( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );
#endif
	_delete_timer( animator );
	
	free( animator );
	
}



/**
 * @ingroup Animator
 *
 * Set the callback function for the animator end.  
 *
 * @param    animator   Animator object 
 * @param    op         Callback function pointer
 * @param    data       Callback function user argument 
 */
EAPI void elm_animator_completion_callback_set( Elm_Animator* animator, void (*op)(void*), void* data )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );
#endif 
	if( animator->on_animating == EINA_TRUE ) {
		return ;
	}
	
	animator->completion_op = op;
	animator->completion_arg = data;
}






/**
 * @ingroup Animator
 *
 * Set the callback function for the animator end. (deprecated. Please try use elm_animator_completion_callback_set) 
 *
 * @param    animator   Animator object 
 * @param    op         Callback function pointer
 * @param    data       Callback function user argument 
 */
EAPI void elm_animator_completion_set( Elm_Animator* animator, void (*op)(void*), void* data )
{
	fprintf( stderr, "elm_animator_completion_set is deprecated!, Try use elm_animator_completion_callback_set!\n" );

	elm_animator_completion_callback_set( animator, op, data );
}



/**
 * @ingroup Animator
 *
 * Stop animator.
 *
 * @param    animator   Animator object 
 */
EAPI void elm_animator_stop( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE
	ELM_FX_NULL_CHECK( animator );
#endif
	animator->on_animating = EINA_FALSE;

	_delete_timer( animator );
	
}


/**
 * @ingroup Animator
 *
 * Set the animator repeat count.
 *
 * @param    animator   Animator object
 * @param    repeat_cnt Repeat count
 */
EAPI void elm_animator_repeat_set( Elm_Animator* animator, const unsigned int repeat_cnt )
{
#ifdef ELM_FX_EXCEPTION_ENABLE
	ELM_FX_NULL_CHECK( animator );

#endif
	if( animator->auto_reverse == EINA_FALSE ) {
		animator->repeat_cnt = repeat_cnt;
	}else {
		animator->repeat_cnt = _animator_compute_reverse_repeat_count( repeat_cnt );
	}
}


/**
 * @ingroup Animator
 *
 * Animate now.
 *
 * @param    animator   Animator object
 */
EAPI void elm_animator_animate( Elm_Animator* animator )
{
#ifdef ELM_FX_EXCEPTION_ENABLE 
	ELM_FX_NULL_CHECK( animator );

	if( animator->animator_op == NULL ) {
		return ;
	}
#endif 
	
	animator->begin_time = ecore_time_get();
	animator->on_animating = EINA_TRUE;
	animator->cur_repeat_cnt = animator->repeat_cnt;

	_delete_timer( animator );

	animator->timer = ecore_timer_add( 0.0016, _animator_animate_cb, animator );
	ecore_timer_interval_set( animator->timer, 0.0016 ); 
		
	if( animator->timer == NULL ) {
		animator->on_animating = EINA_FALSE;
	}
		
}


