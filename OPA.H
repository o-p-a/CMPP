/*
	�悭�g����`�B

		By Ken/ichiro(OPA)
*/

#ifndef OPA_defined
#define OPA_defined
#define __OPA__

#pragma optimize space

								/* �^��`�̗� */
typedef signed char			schar;
typedef unsigned char		uchar;
typedef signed int			sint;
typedef unsigned int		uint;
typedef signed long			slong;
typedef unsigned long		ulong;

typedef enum bool{
	OFF=0,	NO=0,	FALSE=0,	NG=0,	FAIL=0,
	ON=1,	YES=1,	TRUE=1,		OK=1,	DONE=1
}bool;

								/* ���L�̗� */
#define or					:case
#define ncase				break;case
#define ndefault			break;default

#endif
