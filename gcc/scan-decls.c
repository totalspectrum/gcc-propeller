/* scan-decls.c - Extracts declarations from cpp output.
   Copyright (C) 1993, 1995, 1997, 1998,
   1999, 2000 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Per Bothner <bothner@cygnus.com>, July 1993.  */

#include "hconfig.h"
#include "system.h"
#include "cpplib.h"
#include "scan.h"

static void skip_to_closing_brace PARAMS ((cpp_reader *));

int brace_nesting = 0;

/* The first extern_C_braces_length elements of extern_C_braces
   indicate the (brace nesting levels of) left braces that were
   prefixed by extern "C".  */
int extern_C_braces_length = 0;
char extern_C_braces[20];
#define in_extern_C_brace (extern_C_braces_length>0)

/* True if the function declaration currently being scanned is
   prefixed by extern "C".  */
int current_extern_C = 0;

static void
skip_to_closing_brace (pfile)
     cpp_reader *pfile;
{
  int nesting = 1;
  for (;;)
    {
      cpp_token tok;
      enum cpp_ttype token;

      cpp_get_token (pfile, &tok);
      token = tok.type;
      if (token == CPP_EOF)
	break;
      if (token == CPP_OPEN_BRACE)
	nesting++;
      if (token == CPP_CLOSE_BRACE && --nesting == 0)
	break;
    }
}

/* This function scans a C source file (actually, the output of cpp),
   reading from FP.  It looks for function declarations, and
   external variable declarations.  

   The following grammar (as well as some extra stuff) is recognized:

   declaration:
     (decl-specifier)* declarator ("," declarator)* ";"
   decl-specifier:
     identifier
     keyword
     extern "C"
   declarator:
     (ptr-operator)* dname [ "(" argument-declaration-list ")" ]
   ptr-operator:
     ("*" | "&") ("const" | "volatile")*
   dname:
     identifier

Here dname is the actual name being declared.
*/

int
scan_decls (pfile, argc, argv)
     cpp_reader *pfile;
     int argc ATTRIBUTE_UNUSED;
     char **argv ATTRIBUTE_UNUSED;
{
  int saw_extern, saw_inline;
  cpp_token token, prev_id;

 new_statement:
  cpp_get_token (pfile, &token);

 handle_statement:
  current_extern_C = 0;
  saw_extern = 0;
  saw_inline = 0;
  if (token.type == CPP_OPEN_BRACE)
    {
      /* Pop an 'extern "C"' nesting level, if appropriate.  */
      if (extern_C_braces_length
	  && extern_C_braces[extern_C_braces_length - 1] == brace_nesting)
	extern_C_braces_length--;
      brace_nesting--;
      goto new_statement;
    }
  if (token.type == CPP_OPEN_BRACE)
    {
      brace_nesting++;
      goto new_statement;
    }
  if (token.type == CPP_EOF)
    {
      cpp_pop_buffer (pfile);
      if (CPP_BUFFER (pfile) == NULL)
	return 0;

      goto new_statement;
    }
  if (token.type == CPP_SEMICOLON)
    goto new_statement;
  if (token.type != CPP_NAME)
    goto new_statement;

  prev_id.type = CPP_EOF;
  for (;;)
    {
      switch (token.type)
	{
	default:
	  goto handle_statement;
	case CPP_MULT:
	case CPP_AND:
	  /* skip */
	  break;

	case CPP_COMMA:
	case CPP_SEMICOLON:
	  if (prev_id.type != CPP_EOF && saw_extern)
	    {
	      recognized_extern (&prev_id);
	    }
	  if (token.type == CPP_COMMA)
	    break;
	  /* ... fall through ...  */
	case CPP_OPEN_BRACE:  case CPP_CLOSE_BRACE:
	  goto new_statement;
	  
	case CPP_EOF:
	  cpp_pop_buffer (pfile);
	  if (CPP_BUFFER (pfile) == NULL)
	    return 0;
	  break;

	case CPP_OPEN_PAREN:
	  /* Looks like this is the start of a formal parameter list.  */
	  if (prev_id.type != CPP_EOF)
	    {
	      int nesting = 1;
	      int have_arg_list = 0;
	      for (;;)
		{
		  cpp_get_token (pfile, &token);
		  if (token.type == CPP_OPEN_PAREN)
		    nesting++;
		  else if (token.type == CPP_CLOSE_PAREN)
		    {
		      nesting--;
		      if (nesting == 0)
			break;
		    }
		  else if (token.type == CPP_EOF)
		    break;
		  else if (token.type == CPP_NAME
			   || token.type == CPP_ELLIPSIS)
		    have_arg_list = 1;
		}
	      recognized_function (&prev_id, 
				   cpp_get_line (pfile)->line,
				   (saw_inline ? 'I'
				    : in_extern_C_brace || current_extern_C
				    ? 'F' : 'f'), have_arg_list,
				   CPP_BUFFER (pfile)->nominal_fname);
	      cpp_get_token (pfile, &token);
	      if (token.type == CPP_OPEN_BRACE)
		{
		  /* skip body of (normally) inline function */
		  skip_to_closing_brace (pfile);
		  goto new_statement;
		}
	      if (token.type == CPP_SEMICOLON)
		goto new_statement;
	    }
	  break;
	case CPP_NAME:
	  /* "inline" and "extern" are recognized but skipped */
	  if (cpp_ideq (&token, "inline"))
	    {
	      saw_inline = 1;
	    }
	  else if (cpp_ideq (&token, "extern"))
	    {
	      saw_extern = 1;
	      cpp_get_token (pfile, &token);
	      if (token.type == CPP_STRING
		  && token.val.str.len == 1
		  && token.val.str.text[0] == 'C')
		{
		  current_extern_C = 1;
		  cpp_get_token (pfile, &token);
		  if (token.type == CPP_OPEN_BRACE)
		    {
		      brace_nesting++;
		      extern_C_braces[extern_C_braces_length++]
			= brace_nesting;
		      goto new_statement;
		    }
		}
	      else
		continue;
	      break;
	    }
	  /* This may be the name of a variable or function.  */
	  prev_id = token;
	  break;
	}
      cpp_get_token (pfile, &token);
    }
}
