#include <_ansi.h>
#include <reent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include "local.h"

static char __tzname_std[11];
static char __tzname_dst[11];
static char *prev_tzenv = NULL;

/* default to GMT */
char *_tzname[2] = {"GMT" "GMT"};
int _daylight = 0;
time_t _timezone = (time_t)0;

int __tzyear = 0;

int __tznorth = 1;

__tzrule_type __tzrule[2] = { {'J', 0, 0, 0, 0, (time_t)0, 0 }, 
			      {'J', 0, 0, 0, 0, (time_t)0, 0 } };

_VOID
_DEFUN (_tzset_r, (reent_ptr),
        struct _reent *reent_ptr)
{
  char *tzenv;
  int hh, mm, ss, sign, m, w, d, n;
  int i, ch;

  if ((tzenv = _getenv_r (reent_ptr, "TZ")) == NULL)
      {
	TZ_LOCK;
	_timezone = (time_t)0;
	_daylight = 0;
	_tzname[0] = "GMT";
	_tzname[1] = "GMT";
	TZ_UNLOCK;
	return;
      }

  TZ_LOCK;

  if (prev_tzenv != NULL && strcmp(tzenv, prev_tzenv) == 0)
    {
      TZ_UNLOCK;
      return;
    }

  free(prev_tzenv);
  prev_tzenv = _strdup_r (reent_ptr, tzenv);

  /* ignore implementation-specific format specifier */
  if (*tzenv == ':')
    ++tzenv;  

  if (sscanf (tzenv, "%10[^0-9,+-]%n", __tzname_std, &n) <= 0)
    {
      TZ_UNLOCK;
      return;
    }
 
  tzenv += n;

  sign = 1;
  if (*tzenv == '-')
    {
      sign = -1;
      ++tzenv;
    }
  else if (*tzenv == '+')
    ++tzenv;

  mm = 0;
  ss = 0;
 
  if (sscanf (tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) < 1)
    {
      TZ_UNLOCK;
      return;
    }
  
  __tzrule[0].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);
  _tzname[0] = __tzname_std;
  tzenv += n;
  
  if (sscanf (tzenv, "%10[^0-9,+-]%n", __tzname_dst, &n) <= 0)
    {
      _tzname[1] = _tzname[0];
      TZ_UNLOCK;
      return;
    }
  else
    _tzname[1] = __tzname_dst;

  tzenv += n;

  /* otherwise we have a dst name, look for the offset */
  sign = 1;
  if (*tzenv == '-')
    {
      sign = -1;
      ++tzenv;
    }
  else if (*tzenv == '+')
    ++tzenv;

  hh = 0;  
  mm = 0;
  ss = 0;
  
  if (sscanf (tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) <= 0)
    __tzrule[1].offset = __tzrule[0].offset - 3600;
  else
    __tzrule[1].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

  tzenv += n;

  if (*tzenv == ',')
    ++tzenv;

  for (i = 0; i < 2; ++i)
    {
      if (*tzenv == 'M')
	{
	  if (sscanf (tzenv, "M%hu%n.%hu%n.%hu%n", &m, &n, &w, &n, &d, &n) != 3 ||
	      m < 1 || m > 12 || w < 1 || w > 5 || d > 6)
	    {
	      TZ_UNLOCK;
	      return;
	    }
	  
	  __tzrule[i].ch = 'M';
	  __tzrule[i].m = m;
	  __tzrule[i].n = w;
	  __tzrule[i].d = d;
	  
	  tzenv += n;
	}
      else 
	{
	  char *end;
	  if (*tzenv == 'J')
	    {
	      ch = 'J';
	      ++tzenv;
	    }
	  else
	    ch = 'D';
	  
	  d = strtoul (tzenv, &end, 10);
	  
	  /* if unspecified, default to US settings */
	  if (end == tzenv)
	    {
	      if (i == 0)
		{
		  __tzrule[0].ch = 'M';
		  __tzrule[0].m = 4;
		  __tzrule[0].n = 1;
		  __tzrule[0].d = 0;
		}
	      else
		{
		  __tzrule[1].ch = 'M';
		  __tzrule[1].m = 10;
		  __tzrule[1].n = 5;
		  __tzrule[1].d = 0;
		}
	    }
	  else
	    {
	      __tzrule[i].ch = ch;
	      __tzrule[i].d = d;
	    }
	  
	  tzenv = end;
	}
      
      /* default time is 02:00:00 am */
      hh = 2;
      mm = 0;
      ss = 0;
      
      if (*tzenv == '/')
	sscanf (tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n);

      __tzrule[i].s = ss + SECSPERMIN * mm + SECSPERHOUR  * hh;
    }

  __tzcalc_limits (__tzyear);
  _timezone = (time_t)(__tzrule[0].offset);  
  _daylight = __tzrule[0].offset != __tzrule[1].offset;

  TZ_UNLOCK;
}





