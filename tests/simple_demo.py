import string, pprint
import pycrm114 as p

import texts
Alice_frag = \
    "So she was considering in her own mind (as well as she could, for the\n" \
    "hot day made her feel very sleepy and stupid), whether the pleasure\n" \
    "of making a daisy-chain would be worth the trouble of getting up and\n" \
    "picking the daisies, when suddenly a White Rabbit with pink eyes ran\n" \
    "close by her.\n"
Hound_frag = \
    "\"Well, Watson, what do you make of it?\"\n" \
    "Holmes was sitting with his back to me, and I had given him no\n" \
    "sign of my occupation.\n" \
    "\"How did you know what I was doing?  I believe you have eyes in\n" \
    "the back of your head.\"\n"
Macbeth_frag = \
"    Double, double, toil and trouble;\n" \
"    Fire, burn; and cauldron, bubble.\n" \
"    \n" \
"    SECOND WITCH.\n" \
"    Fillet of a fenny snake,\n" \
"    In the caldron boil and bake;\n" \
"    Eye of newt, and toe of frog,\n" \
"    Wool of bat, and tongue of dog,\n" \
"    Adder's fork, and blind-worm's sting,\n" \
"    Lizard's leg, and howlet's wing,--\n" \
"    For a charm of powerful trouble,\n" \
"    Like a hell-broth boil and bubble.\n" \

Willows_frag = \
    "'This is fine!' he said to himself. 'This is better than whitewashing!'\n" \
    "The sunshine struck hot on his fur, soft breezes caressed his heated\n" \
    "brow, and after the seclusion of the cellarage he had lived in so long\n" \
    "the carol of happy birds fell on his dulled hearing almost like a shout."


cb = p.ControlBlock(flags=(p.CRM114_SVM | p.CRM114_STRING),
                    classes=[("Alice", True), ("Macbeth", False)],
                    start_mem = 8000000)

cb.dump(file("test_cb_dump.txt", 'w'))
cb = p.ControlBlock.load(file("test_cb_dump.txt", 'r'))

db = p.DataBlock(cb)

print " Starting to learn the 'Alice in Wonderland' text"
db.learn_text(0, texts.Alice)

print " Starting to learn the 'MacBeth' text"
db.learn_text(1, texts.Macbeth)

print " Writing our datablock as 'simple_demo_datablock.txt'."
db.dump(file("simple_demo_datablock.txt", 'w'))

print " Reading text form back in."
db = p.DataBlock.load(file("simple_demo_datablock.txt", 'r'))

print " Classifying the 'Alice' text."
s = db.classify_text(Alice_frag)
print ("Best match: %s  Tot succ prob: %f  overall_pR: %f  unk_features: %d"
       % (s.best_match(), s.tsprob(), s.overall_pR(), s.unk_features()))
for sc in s.scores():
    print ("documents: %d  features: %d  hits: %d  prob: %f  pR: %f" %
           (sc["documents"], sc["features"], sc["hits"], sc["prob"], sc["pR"]))

print " Classifying the 'Macbeth' text."
s = db.classify_text(Macbeth_frag)
print ("Best match: %s  Tot succ prob: %f  overall_pR: %f  unk_features: %d"
       % (s.best_match(), s.tsprob(), s.overall_pR(), s.unk_features()))
for sc in s.scores():
    print ("documents: %d  features: %d  hits: %d  prob: %f  pR: %f" %
           (sc["documents"], sc["features"], sc["hits"], sc["prob"], sc["pR"]))

print " Classifying the 'Hound' text."
s = db.classify_text(Hound_frag)
print ("Best match: %s  Tot succ prob: %f  overall_pR: %f  unk_features: %d"
       % (s.best_match(), s.tsprob(), s.overall_pR(), s.unk_features()))
for sc in s.scores():
    print ("documents: %d  features: %d  hits: %d  prob: %f  pR: %f" %
           (sc["documents"], sc["features"], sc["hits"], sc["prob"], sc["pR"]))

print " Classifying the 'Wind in the Willows' text."
s = db.classify_text(Willows_frag)
print ("Best match: %s  Tot succ prob: %f  overall_pR: %f  unk_features: %d"
       % (s.best_match(), s.tsprob(), s.overall_pR(), s.unk_features()))
for sc in s.scores():
    print ("documents: %d  features: %d  hits: %d  prob: %f  pR: %f" %
           (sc["documents"], sc["features"], sc["hits"], sc["prob"], sc["pR"]))
