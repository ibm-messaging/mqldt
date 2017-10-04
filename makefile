#<copyright notice="lm-source" pids="" years="2017">
#********************************************************************************
# Copyright (c) 2017 IBM Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# Contributors:
#    Paul Harris - Initial implementation
#********************************************************************************
#</copyright>
#********************************************************************************
#*                                                                              *
#* IBM MQ Log Disk Tester                                                       *
#*                                                                              *
#********************************************************************************
CC=gcc

# Source directory
SRC = src

CFLAGS= -pthread -lm -lrt

# Full list of object files to be created (one for each source file)
OBJS = $(patsubst $(SRC)/%.c, $(SRC)/%.o, $(wildcard $(SRC)/*.c))

mqldt: $(OBJS)
	$(CC) -o mqldt $(OBJS) $(CFLAGS)
	
clean:
	rm $(SRC)/*.o mqldt
	

	

	


