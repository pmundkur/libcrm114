//  Simple example program for libcrm114.
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

//     TIMECHECK lets you run timings on the things going on inside
//     this demo.  You probably want to leave it turned on; it does little harm.
#define TIMECHECK 1
#if TIMECHECK
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#endif


//
//     For this simple example, we'll just use four short excerpts
//     from the original works:
//

static const char Alice_frag[] =
  {
    "So she was considering in her own mind (as well as she could, for the\n"
    "hot day made her feel very sleepy and stupid), whether the pleasure\n"
    "of making a daisy-chain would be worth the trouble of getting up and\n"
    "picking the daisies, when suddenly a White Rabbit with pink eyes ran\n"
    "close by her.\n"
  };


static const char Hound_frag[] =
  {
    "\"Well, Watson, what do you make of it?\"\n"

    "Holmes was sitting with his back to me, and I had given him no\n"
    "sign of my occupation.\n"

    "\"How did you know what I was doing?  I believe you have eyes in\n"
    "the back of your head.\"\n"
  };

static const char Macbeth_frag[] =
  {
"    Double, double, toil and trouble;\n"
"    Fire, burn; and cauldron, bubble.\n"
"    \n"
"    SECOND WITCH.\n"
"    Fillet of a fenny snake,\n"
"    In the caldron boil and bake;\n"
"    Eye of newt, and toe of frog,\n"
"    Wool of bat, and tongue of dog,\n"
"    Adder's fork, and blind-worm's sting,\n"
"    Lizard's leg, and howlet's wing,--\n"
"    For a charm of powerful trouble,\n"
"    Like a hell-broth boil and bubble.\n"
  };

static const char Willows_frag[] =
  {
    "'This is fine!' he said to himself. 'This is better than whitewashing!'\n"
    "The sunshine struck hot on his fur, soft breezes caressed his heated\n"
    "brow, and after the seclusion of the cellarage he had lived in so long\n"
    "the carol of happy birds fell on his dulled hearing almost like a shout."
  };


int main (void)
{

  //
  //    The essential data declarations you will need:
  //
  CRM114_CONTROLBLOCK *p_cb;
  CRM114_DATABLOCK *p_db;
  CRM114_MATCHRESULT result;
  CRM114_ERR err;

  //    Here's the classifier flags we can use (optionally)
  //*************PICK ONE PICK ONE PICK ONE PICK ONE ******************
  // static const long long my_classifier_flags = CRM114_OSB;
  // static const long long my_classifier_flags = CRM114_SVM;
   static const long long my_classifier_flags = (CRM114_SVM | CRM114_STRING);
  // static const long long my_classifier_flags = CRM114_FSCM;
  // static const long long my_classifier_flags = CRM114_HYPERSPACE;
  // static const long long my_classifier_flags = CRM114_ENTROPY;            // toroid
  // static const long long my_classifier_flags = (CRM114_ENTROPY | CRM114_UNIQUE );  // dynamic mesh
  // static const long long my_classifier_flags = (CRM114_ENTROPY | CRM114_UNIQUE | CRM114_CROSSLINK);  // dynamic mesh + reuse

  //     Here's a regex we'll use, just for demonstration sake
  //static const char my_regex[] =
  //{
  //  "[a-zA-Z]+"
  //  ""          //  use this to get default regex
  //};


  //    Here's a valid pipeline (3 words in succession)
  //static const int my_pipeline[UNIFIED_ITERS_MAX][UNIFIED_WINDOW_MAX] =
  //{ {3, 5, 7} };


  //   You'll need these only if you want to do timing tests.
#if TIMECHECK
  struct tms start_time, end_time;
  struct timeval start_val, end_val;
  times ((void *) &start_time);
  gettimeofday ( (void *) &start_val, NULL);
#endif

  printf (" Creating a CB (control block) \n");
  if (((p_cb) = crm114_new_cb()) == NULL)
    {
      printf ("Couldn't allocate!  Must exit!\n");
      exit(0) ;
    };

  //    *** OPTIONAL *** Change the classifier type
  printf (" Setting the classifier flags and style. \n");
  if ( crm114_cb_setflags (p_cb, my_classifier_flags ) != CRM114_OK)
      {
      printf ("Couldn't set flags!  Must exit!\n");
      exit(0);
    };


  printf (" Setting the classifier defaults for this style classifier.\n");
  crm114_cb_setclassdefaults (p_cb);

  //   *** OPTIONAL ***  Change the default regex
  //printf (" Override the default regex to '[a-zA-Z]+' (in my_regex) \n");
  //if (crm114_cb_setregex (p_cb, my_regex, strlen (my_regex)) != CRM114_OK)
  //  {
  //    printf ("Couldn't set regex!  Must exit!\n");
  //    exit(0);
  //  };



  //   *** OPTIONAL *** Change the pipeline from the default
  //printf (" Override the pipeline to be 1 phase of 3 successive words\n");
  //if (crm114_cb_setpipeline (p_cb, 3, 1, my_pipeline) != CRM114_OK)
  //  {
  //    printf ("Couldn't set pipeline!  Must exit!\n");
  //    exit(0);
  //  };



  //   *** OPTIONAL *** Increase the number of classes
  //printf (" Setting the number of classes to 3\n");
  //p_cb->how_many_classes = 2;

  printf (" Setting the class names to 'Alice' and 'Macbeth'\n");
  strcpy (p_cb->class[0].name, "Alice");
  strcpy (p_cb->class[1].name, "Macbeth");
  //strcpy (p_cb->class[2].name, "Hound");

  printf (" Setting our desired space to a total of 8 megabytes \n");
  p_cb->datablock_size = 8000000;


  printf (" Set up the CB internal state for this configuration \n");
  crm114_cb_setblockdefaults(p_cb);

  printf (" Use the CB to create a DB (data block) \n");
  if ((p_db = crm114_new_db (p_cb)) == NULL)
    { printf ("Couldn't create the datablock!  Must exit!\n");
      exit(0);
    };

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif

  printf (" Starting to learn the 'Alice in Wonderland' text\n");
  err = crm114_learn_text(&p_db, 0,
			  Alice,
			  strlen (Alice) );

  printf (" Starting to learn the 'MacBeth' text\n");
  err = crm114_learn_text(&p_db, 1,
			  Macbeth,
			  strlen (Macbeth) );

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif


  //    *** OPTIONAL *** Here's how to read and write the datablocks as 
  //    ASCII text files.  This is NOT recommended for storage (it's ~5x bigger
  //    than the actual datablock, and takes longer to read in as well, 
  //    but rather as a way to debug datablocks, or move a db in a portable fashion
  //    between 32- and 64-bit machines and between Linux and Windows.  
  //
  //    ********* CAUTION *********** It is NOT yet implemented 
  //    for all classifiers (only Markov/OSB, SVM, PCA, Hyperspace, FSCM, and
  //    Bit Entropy so far.  It is NOT implemented yet for neural net, OSBF,
  //    Winnow, or correlation, but those haven't been ported over yet anyway).
  //  

#define READ_WRITE_TEXT
#ifdef READ_WRITE_TEXT
  printf (" Writing our datablock as 'simple_demo_datablock.txt'.\n");
  crm114_db_write_text (p_db, "simple_demo_datablock.txt");
  
  //  printf (" Freeing the old datablock memory space\n");

  printf ("Zeroing old datablock!  Address was %ld\n", (unsigned long) p_db);
  { 
    int i;
    for (i = 0; i < p_db->cb.datablock_size; i++)
      ((char *)p_db)[i] = 0;
  }
  
  //  free (p_db);

  printf (" Reading the text form back in.\n");
  p_db = crm114_db_read_text ("simple_demo_datablock.txt");
  printf ("Created new datablock.  Datablock address is now %ld\n", (unsigned long) p_db);

  

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif 
#endif 




  //    *** OPTIONAL ***  if you want to learn a third class (class #2) do it here
  //printf (" Starting to learn the 'Hound of the Baskervilles' text\n");
  //err = crm114_learn_text(&p_db, 2,
  //			  Hound,
  //			  strlen (Hound) );

  printf ("\n Classifying the 'Alice' text.\n");
  if ((err = crm114_classify_text(p_db,
				  Alice_frag,
				  strlen (Alice_frag),
				  &result))
      == CRM114_OK)
    { crm114_show_result("Alice fragment results", &result); }
    else exit (err);

  printf ("\n Classifying the 'Macbeth' text.\n");
  if ((err = crm114_classify_text(p_db,
				  Macbeth_frag,
				  strlen (Macbeth_frag),
				  &result))
      == CRM114_OK)
    { crm114_show_result( "Macbeth fragment results", &result); }
    else exit (err);


  printf ("\n Classifying the 'Hound' text.\n");
  if ((err = crm114_classify_text(p_db,
				  Hound_frag,
				  strlen (Hound_frag),
				  &result))
      == CRM114_OK)
    { crm114_show_result("Hound fragment results", &result); }
    else exit (err);

  printf ("\n Classifying the 'Wind in the Willows' text.\n");
  if ((err = crm114_classify_text(p_db,
				  Willows_frag,
				  strlen (Willows_frag),
				  &result))
      == CRM114_OK)
    { crm114_show_result( "Wind in the Willows fragment results", &result);}
    else exit (err);

#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
     "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
     end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
     (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
     (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif

  printf (" Freeing the data block and control block\n");
  free (p_db);
  free (p_cb);

  exit (err);
}
