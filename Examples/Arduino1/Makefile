#==========================================================
# Makefile of project to run with 'make' from
# a command line
#
# Works with Arduino IDE 1.x versions
#
#==========================================================

# Change this to the location of the arduino executable
ARDUINO=~/programs/arduino-1.8.19/arduino

PROJECT=Arduino1.ino

# Comment out .SILENT to see the actual build commands
.SILENT:

.PHONY: all all-pre all-post
all: all-pre main all-post

all-pre:
#	echo "Pre..."
	
all-post:
#	echo "Done..."

main:
	echo "  BUILD $@"
	$(ARDUINO) $(PROJECT) --verify

download:
	echo " UPLOAD $@"
	$(ARDUINO) $(PROJECT) --upload


clean:
	echo "  RM .o"

