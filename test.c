//  Test & example program for libcrm114.
//
// Copyright 2010 Kurt Hackenberg & William S. Yerazunis, each individually
// with full rights to relicense.
//
//   This file is part of the CRM114 Library.
//
//   The CRM114 Library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   The CRM114 Library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.
//

#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114_lib.h"
#include "crm114_internal.h"

#include "texts.h"		/* large megatest texts, ~100KB total */

#define TIMECHECK 0
#if TIMECHECK
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#endif



/*
  Set LEARN_APPEND to true (non-zero) to do a sequence of learns as
  CRM114_APPEND CRM114_APPEND ... CRM114_FROMSTART.  Else doesn't mess with
  flags, neither CRM114_APPEND nor CRM114_FROMSTART is set, and classifiers
  solve each time.

  Of course, most classifiers ignore those flags anyway.
*/
#define LEARN_APPEND 1

/* test hack, or set to INT_MAX for no chunking */
#define LEARN_CHUNK_SIZE INT_MAX

static const char Alice_frag[] =
  {
    "So she was considering in her own mind (as well as she could, for the\n"
    "hot day made her feel very sleepy and stupid), whether the pleasure\n"
    "of making a daisy-chain would be worth the trouble of getting up and\n"
    "picking the daisies, when suddenly a White Rabbit with pink eyes ran\n"
    "close by her.\n"
  };

static const char Alice_frag_twisted[] =
  {
    "So she was considering in her own mind (as well as she could, for the\n"
    "COLD day made her feel STUPID AND CONTAGIOUS), whether the pleasure\n"
    "of making a daisy-chain would be worth the trouble of getting up and\n"
    "picking the daisies, when suddenly a White Rabbit with pink eyes ran\n"
    "close by her.\n"
  };

/* lifted from megatest.sh; slightly twisted */
static const char Alice_frag_megatest[] =
  {
    "when suddenly a White Rabbit with pink eyes ran close to her.\n"
  };

static const char Hound_frag[] =
  {
    "\"Well, Watson, what do you make of it?\"\n"

    "Holmes was sitting with his back to me, and I had given him no\n"
    "sign of my occupation.\n"

    "\"How did you know what I was doing?  I believe you have eyes in\n"
    "the back of your head.\"\n"
  };

static const char Hound_frag_twisted[] =
  {
    "\"Well, FRED, what do you make of it?\"\n"

    "Holmes was sitting with his back to me, and I had given him no\n"
    "sign of my occupation.\n"

    "\"How did you know what I was doing?  I believe you have HOLES in\n"
    "the back of your head.\"\n"
  };

/* analogous to megatest.sh Macbeth unknown */
static const char Hound_frag_short_twisted[] =
  {
    "\"Well Watson what do you make of it?\"\n"
  };

static const char Macbeth_frag_megatest[] =
  {
"    Double, double toil and trouble;\n"
"    Fire burn, and cauldron bubble.\n"
  };

static const char Willows_frag_twisted[] =
  {
    "Spring was moving in the air\n"
    "above and in the earth below and around him, penetrating even his dark\n"
    "house with its spirit of divine fever and longing.\n"
  };


/* define the tables that drive the tests */

/* first define the table structures */

struct regex
{
  const char *regex;
  int len;
};

/* a matrix array plus its two run-time dimensions */
struct coeffs
{
  int rows;
  int columns;
  int coeffs[UNIFIED_ITERS_MAX][UNIFIED_WINDOW_MAX];
};

/*
  A document and its name.  An array of these, with the last element
  an end marker (containing a NULL pointer), is a list of documents.
*/
struct document
{
  char name[CLASSNAME_LENGTH + 1];
  const char *text;
  size_t length;
};

/* a set of documents to learn into a class, and the class name */
struct learn_class
{
  char *name;			/* class name */
  int success;			/* T/F: this is a success class */
  const struct document *learn;	/* list of docs to learn into this class */
};

/* a set of texts: array of class learn lists, list of unknowns */
struct document_set
{
  struct learn_class class[CRM114_MAX_CLASSES];
  const struct document *classify; /* list of documents to classify */
};

/*
  A test: a name, a classifier_flags, optional regex and matrix, and a
  text set.  An array of these, with the last element an end marker
  (containing a NULL pointer), is a list of tests.
*/
struct test
{
  char *test_name;
  unsigned long long flags;  /* a classifier and its modifier flags */
  const struct regex *regex;
  const struct coeffs *coeffs;
  const struct document_set *text_set;
};


/* now tables */

/* each of our large documents as a list */

static const struct document Alice_list[] =
  {
    {"Alice",   Alice,   sizeof(Alice)   - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Hound_list[] =
  {
    {"Hound", Hound, sizeof(Hound) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Macbeth_list[] =
  {
    {"Macbeth", Macbeth, sizeof(Macbeth) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Willows_list[] =
  {
    {"Willows", Willows, sizeof(Willows) - 1},
    {"", NULL, 0}	/* list terminator */
  };

/* and the short ones */

static const struct document Alice_frag_list[] =
  {
    {"Alice_frag", Alice_frag, sizeof(Alice_frag) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Alice_frag_twisted_list[] =
  {
    {"Alice_frag_twisted", Alice_frag_twisted, sizeof(Alice_frag_twisted) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Alice_frag_megatest_list[] =
  {
    {"Alice_frag_megatest", Alice_frag_megatest, sizeof(Alice_frag_megatest) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Hound_frag_list[] =
  {
    {"Hound_frag", Hound_frag, sizeof(Hound_frag) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Hound_frag_twisted_list[] =
  {
    {"Hound_frag_twisted", Hound_frag_twisted, sizeof(Hound_frag_twisted) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Hound_frag_short_twisted_list[] =
  {
    {"Hound_frag_short_twisted", Hound_frag_short_twisted, sizeof(Hound_frag_short_twisted) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Macbeth_frag_megatest_list[] =
  {
    {"Macbeth_frag_megatest", Macbeth_frag_megatest, sizeof(Macbeth_frag_megatest) - 1},
    {"", NULL, 0}	/* list terminator */
  };

static const struct document Willows_frag_twisted_list[] =
  {
    {"Willows_frag_twisted", Willows_frag_twisted, sizeof(Willows_frag_twisted) - 1},
    {"", NULL, 0}	/* list terminator */
  };

/* tables to emulate megatest.sh */

static const struct document megatest_unknown[] =
  {
    {"Alice_frag_megatest",   Alice_frag_megatest,   sizeof(Alice_frag_megatest) - 1},
    {"Macbeth_frag_megatest", Macbeth_frag_megatest, sizeof(Macbeth_frag_megatest) - 1},
    {"", NULL, 0} /* list terminator */
  };

static const struct document_set megatest_set =
  {
    {
      {"Macbeth", TRUE,  Macbeth_list},
      {"Alice",   FALSE, Alice_list}
    },
    megatest_unknown
  };

static const struct regex three_letter_regex = {"\\w\\w\\w", 6};

static const struct coeffs three_word_matrix =
  {
    1, 3,
    {{1, 1, 1,}}
  };

/*
  megatest.sh sets regex [[:graph:]]+
  here we set no regex, which causes an internal emulation of that
*/
static const struct test megatest[] =
  {
    /* default is Markov in engine, OSB Markov (OSB Bayes) in library */
    {"megatest default",                        (unsigned long long)0,                      NULL, NULL, &megatest_set},
#if 0	/* if Markov has been converted */
    {"megatest SBPH Markov",                    CRM114_MARKOVIAN,                              NULL, NULL, &megatest_set},
#endif
    {"megatest OSB Markov",                     CRM114_OSB_BAYES,                              NULL, NULL, &megatest_set},
    {"megatest OSB Markov unique",              CRM114_OSB_BAYES | CRM114_UNIQUE,                 NULL, NULL, &megatest_set},
    {"megatest OSB Markov unique chi squared",  CRM114_OSB_BAYES | CRM114_UNIQUE | CRM114_CHI2,      NULL, NULL, &megatest_set},
#if 0
    {"megatest OSBF local confidence (Fidelis)",CRM114_OSBF,                                   NULL, NULL, &megatest_set},
    /*
      Next negatest.sh does winnow, which hasn't been libraryized, but
      the megatest.sh winnow test learns one document and refutes the
      other into each class, which this program doesn't know how to
      do.  Never mind.
    */
#endif
    /*
      Now megatest.sh uses default classifier and unigram.  Default is
      different in library, but I guess unigram makes it effectively
      the same.
    */
    {"megatest default unigram",                CRM114_UNIGRAM,                                NULL, NULL, &megatest_set},
    /* next megatest.sh does winnow (unigram), not converted, we can't learn/refute anyway */
    /*
      In the old (engine) hyperspace:
	   learned features are one too high
	   unknown features are one too low
	   hits are a little low, by varying amounts
      Incorrect # unknown features is used in the classify, so the
      last unknown feature is never seen, and radiance, probability,
      and pR can be off.
    */
    /*
      Next megatest.sh does hyperspace, learn unique but classify not
      unique, which I suspect is a bug in the script.  This program
      can't do it anyway -- can't use different flags for learn and
      classify (it's bug-proof!).
    */
    /* now one we can do */
    {"megatest hyperspace 3-letter regex unigram", CRM114_HYPERSPACE | CRM114_UNIGRAM,            &three_letter_regex, NULL, &megatest_set},
    /* Next megatest.sh does hyperspace, learn unique unigram, classify unigram, maybe a bug.  We can't do it. */
    {"megatest hyperspace string",              CRM114_HYPERSPACE | CRM114_STRING,                NULL, NULL, &megatest_set},
    {"megatest hyperspace string unigram",      CRM114_HYPERSPACE | CRM114_STRING | CRM114_UNIGRAM,  NULL, NULL, &megatest_set},
    {"megatest hyperspace 3-word matrix",       CRM114_HYPERSPACE,                             NULL, &three_word_matrix, &megatest_set},
    {"megatest bit entropy unique crosslink",   CRM114_ENTROPY   | CRM114_UNIQUE | CRM114_CROSSLINK, NULL, NULL, &megatest_set},
    {"megatest bit entropy toroid",             CRM114_ENTROPY,                                NULL, NULL, &megatest_set},
    {"megatest FSCM",                           CRM114_FSCM,                                   NULL, NULL, &megatest_set},
#if 0	/* if classifiers converted */
    /* megatest.sh does neural net, append, refute, fromstart, different unknowns -- never mind */
    /* megatest.sh does neural alternating example */
#endif
    /*
      Next megatest.sh does two SVM tests, unigram unique and unique,
      that learn just one paragraph from each large document.  We
      could do it here but haven't bothered.
    */
    /* megatest.sh does SVM alternating example */
    /* megatest.sh does SKS one paragraph, with numeric parameters we can't pass on */
    /* megatest.sh does SKS one paragraph unique with parameters and translated text */
#if 0	/* if classifiers converted */
    /* megatest.sh does correlate, no learn, just classify, different unknown */
    /* clump/pmulc */
#endif
    {"megatest PCA unigram unique",             CRM114_PCA | CRM114_UNIGRAM | CRM114_UNIQUE,   NULL, NULL, &megatest_set},
    {"megatest PCA unique",                     CRM114_PCA | CRM114_UNIQUE,                    NULL, NULL, &megatest_set},
    /* PCA alternating example */
    {"", 0, NULL, NULL, NULL}
  };

static const struct test like_megatest[] =
  {
    {"like megatest OSB Markov unigram",             CRM114_OSB_BAYES | CRM114_UNIGRAM,                   NULL, NULL, &megatest_set},
    {"like megatest hyperspace",                     CRM114_HYPERSPACE,                                   NULL, NULL, &megatest_set},
    {"like megatest SVM unigram unique",             CRM114_SVM | CRM114_UNIGRAM | CRM114_UNIQUE,         NULL, NULL, &megatest_set},
    {"like megatest SVM unique",                     CRM114_SVM | CRM114_UNIQUE,                          NULL, NULL, &megatest_set},
    {"like megatest SVM",                            CRM114_SVM,                                          NULL, NULL, &megatest_set},
    {"like megatest PCA",                            CRM114_PCA,                                          NULL, NULL, &megatest_set},
    {"", 0, NULL, NULL, NULL}
  };


/* all our short texts to classify, used in several tests */

static const struct document all_unknown[] =
  {
    {"Alice_frag",               Alice_frag,               sizeof(Alice_frag) - 1},
    {"Alice_frag_twisted",       Alice_frag_twisted,       sizeof(Alice_frag_twisted) - 1},
    {"Alice_frag_megatest",      Alice_frag_megatest,      sizeof(Alice_frag_megatest) - 1},
    {"Hound_frag",               Hound_frag,               sizeof(Hound_frag) - 1},
    {"Hound_frag_twisted",       Hound_frag_twisted,       sizeof(Hound_frag_twisted) - 1},
    {"Hound_frag_short_twisted", Hound_frag_short_twisted, sizeof(Hound_frag_short_twisted) - 1},
    {"Macbeth_frag_megatest",    Macbeth_frag_megatest,    sizeof(Macbeth_frag_megatest) - 1},
    {"Willows_frag_twisted",     Willows_frag_twisted,     sizeof(Willows_frag_twisted) - 1},
    {"", NULL, 0}
  };


/*
  Now a test with more than one learned document per class.
  Were the unknowns written for children, or for adults?
*/

static const struct document child_list[] =
  {
    {"Alice",   Alice,   sizeof(Alice) - 1},
    {"Willows", Willows, sizeof(Willows) - 1},
    {"", NULL, 0}
  };

static const struct document adult_list[] =
  {
    {"Hound",   Hound,   sizeof(Hound) - 1},
    {"Macbeth", Macbeth, sizeof(Macbeth) - 1},
    {"", NULL, 0}
  };

static const struct document_set reader_set =
  {
    {
      {"child", TRUE,  child_list},
      {"adult", FALSE, adult_list},
    },
    all_unknown
  };

static const struct test reader[] =
  {
    {"adult/child OSB Bayes",                    CRM114_OSB_BAYES,                            NULL, NULL, &reader_set},
    {"adult/child bit entropy unique crosslink", CRM114_ENTROPY | CRM114_UNIQUE | CRM114_CROSSLINK, NULL, NULL, &reader_set},
    {"adult/child hyperspace",                   CRM114_HYPERSPACE,                           NULL, NULL, &reader_set},
    {"adult/child SVM",                          CRM114_SVM,                                  NULL, NULL, &reader_set},
    {"adult/child PCA",                          CRM114_PCA,                                  NULL, NULL, &reader_set},
    {"adult/child FSCM",                         CRM114_FSCM,                                 NULL, NULL, &reader_set},
    {"", 0, NULL, NULL, NULL}
  };


/*
  Test the mechanism for supplying regex and matrix.
  These two tests should give the same results.
 */

static const char dr_str[] = {"[[:graph:]]+"};
static const struct regex default_regex = {dr_str, sizeof(dr_str) - 1};

static const struct coeffs osb_coeffs =
  {
    4, OSB_BAYES_WINDOW_LEN,
    {
      {1, 3, 0,  0,  0},
      {1, 0, 5,  0,  0},
      {1, 0, 0, 11,  0},
      {1, 0, 0,  0, 23},
    }
  };

static const struct test regex_coeff[] =
  {
    {"adult/child OSB Bayes, default regex and matrix",               CRM114_OSB_BAYES, NULL,           NULL,        &reader_set},
    {"adult/child OSB Bayes, supply regex & matrix same as defaults", CRM114_OSB_BAYES, &default_regex, &osb_coeffs, &reader_set},
    {"", 0, NULL, NULL, NULL}
  };

/* now a different kind of test: four classes, all success */

static const struct document_set four_class_set =
  {
    {
      {"Alice",   TRUE, Alice_list},
      {"Hound",   TRUE, Hound_list},
      {"Macbeth", TRUE, Macbeth_list},
      {"Willows", TRUE, Willows_list},
    },
    all_unknown
  };

static const struct test four_class[] =
  {
    {"four-way OSB Markov",                   CRM114_OSB_BAYES,                            NULL, NULL, &four_class_set},
    {"four-way bit entropy unique crosslink", CRM114_ENTROPY | CRM114_UNIQUE | CRM114_CROSSLINK, NULL, NULL, &four_class_set},
    {"four-way Hyperspace",                   CRM114_HYPERSPACE,                           NULL, NULL, &four_class_set},
    {"four-way FSCM",                         CRM114_FSCM,                                 NULL, NULL, &four_class_set},
    {"", 0, NULL, NULL, NULL}
  };


/* four classes: two success, two failure */

static const struct document_set two_two_set =
  {
    {
      {"Alice",   TRUE,  Alice_list},
      {"Hound",   FALSE, Hound_list},
      {"Macbeth", FALSE, Macbeth_list},
      {"Willows", TRUE,  Willows_list},
    },
    all_unknown
  };

static const struct test two_two[] =
  {
    {"two and two OSB Markov",                   CRM114_OSB_BAYES,                            NULL, NULL, &two_two_set},
    {"two and two bit entropy unique crosslink", CRM114_ENTROPY | CRM114_UNIQUE | CRM114_CROSSLINK, NULL, NULL, &two_two_set},
    {"two and two hyperspace",                   CRM114_HYPERSPACE,                           NULL, NULL, &two_two_set},
    {"two and two FSCM",                         CRM114_FSCM,                                 NULL, NULL, &two_two_set},
    {"", 0, NULL, NULL, NULL}
  };


/* try giving a funky regex */

static const struct regex dot = {".", 1};

static const struct test two_two_regex[] =
  {
    {"two and two default, \".\"", (unsigned long long)0, &dot, NULL, &two_two_set},
    {"", 0, NULL, NULL, NULL}
  };


/* debug hack */

static const char dog[] = "dog";
static const char cat[] = "cat";
static const char hamster[] = "hamster";
static const char cow[] = "cow";
static const char snake[] = "snake";

static const struct document mammal_list[] =
  {
    {"dog", dog,  sizeof(dog) - 1},
    {"cat", cat,  sizeof(cat) - 1},
    {"hamster", hamster, sizeof(hamster) - 1},
    {"cow", cow, sizeof(cow) - 1},
    {"", NULL, 0}
  };

static const struct document reptile_list[] =
  {
    {"snake", snake, sizeof(snake) - 1},
    {"", NULL, 0}
  };

static const struct document vertebrate_list[] =
  {
    {"dog", dog,  sizeof(dog) - 1},
    {"cat", cat,  sizeof(cat) - 1},
    {"hamster", hamster, sizeof(hamster) - 1},
    {"cow", cow, sizeof(cow) - 1},
    {"snake", snake, sizeof(snake) - 1},
    {"", NULL, 0}
  };

static const struct document_set animal_set =
  {
    {
      /* just one class -- that should confuse things */
      {"mammal", TRUE, mammal_list}
    },
    vertebrate_list
  };

static const struct test animal[] =
  {
    {"animal hyperspace unigram", CRM114_HYPERSPACE | CRM114_UNIGRAM, NULL, NULL, &animal_set},
    {"", 0, NULL, NULL, NULL}
  };


/* test the tester */

static const struct document_set unbalanced_animal_set =
  {
    {
      /* learn docs / class differ by more than 1 */
      {"mammal", TRUE, mammal_list},
      {"reptile", FALSE, reptile_list},
    },
    vertebrate_list
  };

static const struct test unbalanced_animal[] =
  {
    {"unbalanced animal hyperspace unigram", CRM114_HYPERSPACE | CRM114_UNIGRAM, NULL, NULL, &unbalanced_animal_set},
    {"", 0, NULL, NULL, NULL}
  };


/* functions to execute the above tests */

typedef CRM114_ERR (LEARN_FUNC)(CRM114_DATABLOCK **,
				const struct test *);

static LEARN_FUNC learn_sequential;
static LEARN_FUNC learn_round_robin;


static CRM114_ERR set_up_cb(CRM114_CONTROLBLOCK *p_cb, const struct test *t)
{
  CRM114_ERR err;
  int c;

  if ((err = crm114_cb_setflags(p_cb, t->flags)) == CRM114_OK
      && (t->regex == NULL
	  || (err = crm114_cb_setregex(p_cb, t->regex->regex,
				       t->regex->len)) == CRM114_OK)
      && (t->coeffs == NULL
	  || (err = crm114_cb_setpipeline(p_cb, t->coeffs->columns,
					  t->coeffs->rows,
					  t->coeffs->coeffs)) == CRM114_OK))
    switch (p_cb->classifier_flags & CRM114_FLAGS_CLASSIFIERS_MASK)
      {
      case CRM114_SVM:
      case CRM114_PCA:
	crm114_cb_setclassdefaults(p_cb);
	for (c = 0;
	     c < p_cb->how_many_classes
	       && t->text_set->class[c].learn != NULL;
	     c++)
	  {
	    p_cb->class[c].success = t->text_set->class[c].success;
	    /* set class's name */
	    crm114__strn1cpy(p_cb->class[c].name, t->text_set->class[c].name,
			     sizeof(p_cb->class[c].name));
	  }
	break;
      default:
	/* For classifiers that handle multiple classes. */
	for (c = 0;
	     c < CRM114_MAX_CLASSES && t->text_set->class[c].learn != NULL;
	     c++)
	  {
	    p_cb->class[c].success = t->text_set->class[c].success;
	    /* set class's name */
	    crm114__strn1cpy(p_cb->class[c].name, t->text_set->class[c].name,
			     sizeof(p_cb->class[c].name));
	  }
	p_cb->how_many_classes = c;
	crm114_cb_setblockdefaults(p_cb);
	break;
      }

  return err;
}


static CRM114_ERR learn_sequential(CRM114_DATABLOCK **db, const struct test *t)
{
  CRM114_ERR err;
  int c;			/* class subscript */
  int ld;			/* learn-document subscript */
  size_t ofs;			/* offset in a document */

  /* classes are consecutive from 0, no gaps */
  /* for each class to be learned */
  for (err = CRM114_OK, c = 0;
       err == CRM114_OK && c < CRM114_MAX_CLASSES
	 && t->text_set->class[c].learn != NULL;
       c++)
    /* learn class's documents */
    for (ld = 0;
	 err == CRM114_OK && t->text_set->class[c].learn[ld].text != NULL;
	 ld++)
      {
	struct document learn = t->text_set->class[c].learn[ld];

	/*
	  optionally hack up each learned document into
	  arbitrary-sized chunks, to test with a lot of docs
	*/
	for (ofs = 0; err == CRM114_OK && ofs < learn.length;
	     ofs += LEARN_CHUNK_SIZE)
	  {
#if LEARN_APPEND
	    /*
	      duplicate tests of all 3 nested loops
	      this must change if any loop changes
	      test: if not doing last last last...
	    */
	    if ( !((c + 1 >= CRM114_MAX_CLASSES
		    || t->text_set->class[c + 1].learn == NULL)
		   && t->text_set->class[c].learn[ld + 1].text == NULL
		   && ofs + LEARN_CHUNK_SIZE >= learn.length))
	      /* not last learn, just append it */
	      (*db)->cb.classifier_flags |= CRM114_APPEND;
	    else
	      /* last learn, learn it and solve everything */
	      (*db)->cb.classifier_flags =
		((*db)->cb.classifier_flags & ~CRM114_APPEND) | CRM114_FROMSTART;
#endif
	    err = crm114_learn_text(db, c, &learn.text[ofs],
				    MIN(learn.length - ofs, LEARN_CHUNK_SIZE));
#if LEARN_APPEND
	    (*db)->cb.classifier_flags &= ~(CRM114_APPEND | CRM114_FROMSTART);
#endif
	  }
      }

  return err;
}


static CRM114_ERR learn_round_robin(CRM114_DATABLOCK **db, const struct test *t)
{
  int nld[CRM114_MAX_CLASSES];	/* number of learn docs in each class */
  CRM114_ERR err;
  int ld;			/* learn document subscript */
  size_t ofs;			/* offset within documents */
  int c;			/* class subscript */
  int found_doc;		/* T/F */
  int found_chunk;		/* T/F */
#if LEARN_APPEND
  int nlearn;
  int ilearn;
#endif

  /* remember how many documents to learn in each class */
  for (c = 0; c < CRM114_MAX_CLASSES; c++)
    if (t->text_set->class[c].learn != NULL)
      for (nld[c] = 0; t->text_set->class[c].learn[nld[c]].text != NULL;
	   nld[c]++)
	;
    else
      nld[c] = 0;

#if LEARN_APPEND
  /*
    Here's sort of a kluge.  We can't loop through the structure in
    round robin order and know when we're on the last learn.  We don't
    know we're done until later, after more searching.  So, to put
    CRM114_FROMSTART on the last learn, we first count the learn chunks
    in the test structure, and then while doing learns just count up
    to that number.  Crude, simple, works.

    This counting doesn't have to be done in the same order as the
    learns, so we do it in the natural sequential order -- the easy
    order.
  */

  nlearn = 0;
  for (c = 0;
       c < CRM114_MAX_CLASSES && t->text_set->class[c].learn != NULL;
       c++)
    for (ld = 0; t->text_set->class[c].learn[ld].text != NULL; ld++)
      for (ofs = 0; ofs < t->text_set->class[c].learn[ld].length;
	   ofs += LEARN_CHUNK_SIZE)
	nlearn++;

  ilearn = 0;
#endif

  for (err = CRM114_OK, ld = 0; err == CRM114_OK; ld++)
    {
      for (ofs = 0, found_chunk = TRUE; err == CRM114_OK;
	   ofs += LEARN_CHUNK_SIZE)
	{
	  for (c = 0, found_doc = found_chunk = FALSE;
	       err == CRM114_OK && c < CRM114_MAX_CLASSES
		 && t->text_set->class[c].learn != NULL;
	       c++)
	    /*
	      we need this kluge with nld so we don't go beyond the
	      end-of-list marker in any class's list of documents to
	      learn
	    */
	    if (ld < nld[c])
	      {
		/* found a doc[ld] in any class */
		found_doc = TRUE;

		if (ofs < t->text_set->class[c].learn[ld].length)
		  {
		    struct document learn = t->text_set->class[c].learn[ld];

		    /* found a chunk (not past doc end) in any class */
		    found_chunk = TRUE;
#if LEARN_APPEND
		    /* if this is not the last learn */
		    if (ilearn != nlearn - 1)
		      (*db)->cb.classifier_flags |= CRM114_APPEND;
		    else
		      (*db)->cb.classifier_flags =
			((*db)->cb.classifier_flags & ~CRM114_APPEND)
			| CRM114_FROMSTART;
#endif

		    /* finally, after all this complication, learn something */
		    err = crm114_learn_text(db, c, &learn.text[ofs],
					    MIN(learn.length - ofs,
						LEARN_CHUNK_SIZE));
#if LEARN_APPEND
		    (*db)->cb.classifier_flags &= ~(CRM114_APPEND | CRM114_FROMSTART);
		    ilearn++;
#endif
		  }
	      }


	  if ( !found_chunk)
	    break;
	}

      if ( !found_doc)
	break;
    }

  return err;
}


/*
  Do one test.  With specified classifier, flags, regex, and matrix of
  coefficients, learn some documents and then classify some documents.
*/
static CRM114_ERR test(const struct test *t, LEARN_FUNC *learn_func)
{
  CRM114_CONTROLBLOCK *p_cb;
  CRM114_DATABLOCK *p_db;
  CRM114_MATCHRESULT result;
  CRM114_ERR err;
  int i;

#if TIMECHECK
  struct tms start_time, end_time;
  struct timeval start_val, end_val;
#endif
  //               Speed verification if desired
#if TIMECHECK
  times ((void *) &start_time);
  gettimeofday ( (void *) &start_val, NULL);
#endif

  printf("flags: %#18llx  ( %s%s )\n", t->flags, t->test_name,
	 (learn_func == learn_sequential) ? " learn sequential"
	 : ((learn_func == learn_round_robin) ? " learn round robin"
	    : ""));

  if ((p_cb = crm114_new_cb()) != NULL)
    {
      if ((err = set_up_cb(p_cb, t)) == CRM114_OK)
	{
	  if ((p_db = crm114_new_db(p_cb)) != NULL)
	    {
	      err = (*learn_func)(&p_db, t);
#if 0
	      /* test hack */
	      /*
		Having done all the learns, write the DB into a text
		file, read that text file back into a new temp DB,
		write that second DB into a second text file, compare
		the two text files.

		Leave the text files lying around to look at.
	      */
	      {
		char filename[100];
		char filename_copy[100];
		char cmd[300];
		CRM114_DATABLOCK *tmpdb;

		sprintf(filename, "/tmp/%llx", p_db->cb.classifier_flags);
		sprintf(filename_copy, "/tmp/%llx_copy", p_db->cb.classifier_flags);
		sprintf(cmd, "cmp -s %s %s", filename, filename_copy);
		if (crm114_db_write_text(p_db, filename))
		  if ((tmpdb = crm114_db_read_text(filename)) != NULL)
		    {
		      (void)crm114_db_write_text(tmpdb, filename_copy);
		      if (memcmp(&p_db->cb, &tmpdb->cb, sizeof(p_db->cb)) == 0
			  && system(cmd) == 0)
			printf("OK!\n");
		      else
			printf("mismatch\n");

		      free(tmpdb);
		    }
		  else
		    printf("read failed\n");
		else
		  printf("write failed\n");
		}
#endif
	      /* if no error, do all the classifies */
	      for (i = 0;
		   err == CRM114_OK && t->text_set->classify[i].text != NULL;
		   i++)
		{
		  char resultname[129];
		  struct document classify = t->text_set->classify[i];
		  if ((err = crm114_classify_text(p_db, classify.text,
						  classify.length, &result))
		      == CRM114_OK)
		    {
		      resultname[0] = '\000';
		      strcat (resultname, classify.name);
		      strcat (resultname, " : \n");
		      crm114_show_result(resultname, &result);
		    }
#if 0
		  /* test hack */
		  /*
		    After each classify, write the DB into a text
		    file, read the text file into a new temp DB, do
		    the classify again with the new DB and a new
		    result structure, and compare the two result
		    structures.

		    Deletes the text files.
		  */
		  if (err == CRM114_OK)
		    {
		      FILE *tmpf;
		      CRM114_DATABLOCK *tmpdb;
		      CRM114_MATCHRESULT tmpresult;

		      if ((tmpf = tmpfile()) != NULL)
			{
			  if (crm114_db_write_text_fp(p_db, tmpf))
			    {
			      rewind(tmpf);
			      if ((tmpdb = crm114_db_read_text_fp(tmpf)) != NULL)
				{
				if (crm114_classify_text(tmpdb, classify.text, classify.length, &tmpresult) == CRM114_OK)
				  if (memcmp(&result, &tmpresult, sizeof(result)) == 0)
				    printf("OK!\n");
				  else
				    printf("mismatch\n");
				else
				  printf("second classify failed\n");

				free(tmpdb);
				}
			      else
				printf("read text failed\n");
			    }
			  else
			    printf("write text failed\n");

			  fclose(tmpf);
			}
		      else
			printf("tmpfile() failed\n");
		    }
#endif
		}

	      free(p_db);
	    }
	  else
	    err = CRM114_UNK;
	}

      free(p_cb);
    }
  else
    err = CRM114_UNK;

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf ("Elapsed time for test %s : %9.6f total, %6.3f user, %6.3f system.\n",
	  t->test_name,
	  end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
	  (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
	  (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif

  return err;
}


/*
  Execute a list of tests (an array of struct test, terminated by one
  containing a NULL pointer).  Each test is classifier flags and two
  sets of documents.
 */

static CRM114_ERR test_list(const struct test list[], LEARN_FUNC *learn_func)
{
  CRM114_ERR err;
  int i;

  /*
    We terminate this loop when the pointer list[i].text_set is NULL.
    (A text set is the tuple of training documents and test
    documents.)
  */
  for (err = CRM114_OK, i = 0;
       err == CRM114_OK && list[i].text_set != NULL;
       i++)
    {
      err = test(&list[i], learn_func);
      printf("\n");
    }
  if (err != CRM114_OK)
    printf("err: %d\n", err);

  return err;
}


/* Main program to run testing. */

int main(void)
{
  (void)test_list(megatest,          learn_round_robin);
  (void)test_list(like_megatest,     learn_round_robin);
  (void)test_list(reader,            learn_round_robin);
  (void)test_list(regex_coeff,       learn_round_robin);
  (void)test_list(four_class,        learn_round_robin);
  (void)test_list(two_two,           learn_round_robin);
  (void)test_list(two_two_regex,     learn_round_robin);
  (void)test_list(animal,            learn_round_robin);
  (void)test_list(unbalanced_animal, learn_round_robin);

  return 0;
}
