#!/bin/bash

if [[ ! -f asciidraw.c ]]
then
	echo "Error cannot locate asciidraw.c"
	exit 1
fi

# Modify this to whatever is your compilation statement is (subject to the restrictions in the assignment PDF)
gcc -o asciidraw asciidraw.c -lm
rc=$?

if [[ $rc -ne 0 ]]
then
	echo "There were errors/warnings from gcc. rc = $rc"
	exit $rc
fi

echo " --- test case to draw rectangles, etc --- "
echo '
/asciidraw <<ENDOFCMDS
GRID 40 40
RECTANGLE 2,15 25,5
RECTANGLE 30,45 35,35
DISPLAY
END
ENDOFCMDS
'
./asciidraw <<ENDOFCMDS
GRID 40 40
RECTANGLE 2,15 25,5
RECTANGLE 30,45 35,35
DISPLAY
END
ENDOFCMDS

echo " --- test cases to draw circles --- "
echo '
/asciidraw <<ENDOFCMDS
GRID 100
CIRCLe 50,50 25
CIRCLE 50,50 25
CIRCLE 5,10 10
CIRCLE 45,10
DISPLAY
CIRCLE 50,5O 30
CIRCLE 0,0 30
DISPLAY
END
ENDOFCMDS
'
./asciidraw <<ENDOFCMDS
GRID 100 100
CInfabifb
CIRCLE 50,50 25
CIRCLE 5,10 10
CIRCLE 45,10
DISPLAY
CIRCLE 50,5O 30
CIRCLE 0,0 30
DISPLAY
END
ENDOFCMDS

echo " --- test cases to draw lines --- "
echo '
/asciidraw <<ENDOFCMDS
GRID 50 50
LINE 2,15 25,5
LINE 10,20 20,30
LINE 10,10 30,45
LINE 10,45 30,10
DISPLAY
LINE 25,5 2,15
LINE 20,30 10,20
LINE 30,45 10,10
LINE 30,10 10,45
END
ENDOFCMDS
'
./asciidraw <<ENDOFCMDS
GRID 50 50
LINE 2,15 25,5
LINE 10,20 20,30
LINE 10,10 30,45
LINE 10,45 30,10
DISPLAY
LINE 25,5 2,15
LINE 20,30 10,20
LINE 30,45 10,10
LINE 30,10 10,45
DISPLAY
END
ENDOFCMDS

echo " --- test cases to draw various shapes--- "
echo '
/asciidraw <<ENDOFCMDS
GRID 50 70
LINE 2,15 25,5
RECTANGLE 10,40 30,5
CHAR - 
DISPLAY
LINE 25,15 2,15
LINE 20,64 10,17
CHAR +
LINE 30,74 10,10
DISPLAY
END
ENDOFCMDS
'
./asciidraw <<ENDOFCMDS
GRID 50 70
LINE 2,15 25,5
Rectangle 10,20 20,30
RECTANGLE 10,40 30,5
CHAR - 
DISPLAY
LINE 25,15 2,15
LINE 20,64 10,17
CHAR +
LINE 30,74 10,10
DISPLAY
END
ENDOFCMDS
