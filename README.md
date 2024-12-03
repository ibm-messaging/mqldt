# mqldt - IBM MQ Log Disk Tester

The purpose of this tool is to test the capability of a Linux mounted filesystem being used (or being proposed) to host an MQ recovery log.

![#f03c15](https://placehold.it/15/f03c15/000000?text=+)<b> Warning!!</b>  
<ul><li>If any queue manager is currently using the filesystem being tested:</li>
<ul><li>Do NOT specify existing log files to write to.</li>
<li>Create a new directory on the filesystem to be tested (this must be in the same file system as the current MQ logs)</li>
<li>Running this tool WILL adversely affect the performance of the existing queue manager(s), for the duration of the test, (which may, in turn, cause applications problems whose symptoms last beyond the filesystem test.</li>
</ul></ul>

The tool is similar in purpose to fio, but has been written to test writing to a filesystem with the same options, and in the same way, that MQ writes to its recovery logs, without having to specify lots of options on the command line to get it right.

There are additionally, some problems with the use of fio, which this tool addresses (like being unable to get the open and write options exactly the same as MQ, and the pre-loading and usage pattern of the log files).

What this tool cannot do, is tell you exactly what performance you will get from your persistent MQ workload, even when log writes are the limiting factor. The workload on a machine can impact the performance of the IOPs, especially on modern multi-core machines. The best solution is always to test MQ itself.

<pre>
Usage:

mqldt &ltoptions&gt

Options:
--dir         : Directory in which create and test files being written to. 
--filePrefix  : prefix of test files
--bsize       : Blocksize(s), of writes to be used. Can be a single value, or a comma separated list. Supports k suffix.
--fileSize    : File size of each test file
--numFiles    : Number of files to create and use
--duration    : Number of seconds to test
--csvFile     : Optional csv output file
--qm          : Number of Queue Managers
--delay       : Delay to add to each IO write (microsec)
</pre>
e.g.
mqldt --dir=/var/san1/testdir --filePrefix=mqtestfile --bsize=5K,9K,17K,24K,49K,48K,54K,62K,77K,95K,105K --fileSize=67108864 --numFiles=16 --duration=40 --csvFile=./san_log.csv

Use the values in qm.ini (or those planned) to calculate some of the parameters to MQLDT.

<table>
<tr><td>MQLDT Parm</td><td>qm.ini parm(s)</td></tr>
<tr><td>--dir</td><td>Any directory hosted by the same file-system as <i>Log:LogPath</i></td></tr>
<tr><td>--fileSize</td><td><i>Log:LogFilePages</i> x 4096</td></tr>
<tr><td>--numFiles</td><td><i>Log:LogPrimaryFiles</i></td></tr>
</table>

## Sample output:

<pre>
Options (specified or defaulted to)
=====================================================================================
Write blocksize             (--bsize)              : 128K
Directory to write to       (--dir)                : /var/san1/testdir
Test file prefix            (--filePrefix)         : mqtestfile
Number of files to write to (--numFiles)           : 24
Size of test files          (--fileSize)           : 67108864
Test duration               (--duration)           : 20

Creating files...
Executing test for write blocksize 131072 (128k). Seconds elapsed -> 20/20

Total writes to files                                :          46,178
Total bytes written to files                         :   6,052,642,816
Max bytes/sec written to files (over 1 sec interval) :     311,689,216
Min bytes/sec written to files (over 1 sec interval) :     297,795,584
Avg bytes/sec written to files                       :     302,732,356

Max latency of write (ns)      :       4,977,438 (#24,649)
Min bytes/sec (slowest write)  :      26,333,226
Min latency of write (ns)      :         375,889 (#42,421)
Max bytes/sec (fastest write)  :     348,698,688
Avg latency of write (ns)      :       4,935,648
</pre>

## Description of Output
<table>
<tr><td>Total writes to files</td><td>The total number of writev operations completed over the duration of the test</td></tr>
<tr><td>IOPS</td><td>The total number of writev operations completed per second over the duration of the test</td></tr>
<tr><td>Total bytes written to files</td><td>Total bytes written over the duration of the test (i.e Total writes to files x blocksize)</td></tr>
<tr><td>Max bytes/sec written to files (over 1 sec interval)</td><td>MQLDT calculates how much data has been written every second, this is the maximum write bandwidth measured for any one second duration. If the test is only run for one second this number will be the same as 'Avg bytes/sec written to files'</td></tr>
<tr><td>Min bytes/sec written to files (over 1 sec interval)</td><td>This is the minimum write bandwidth measured for any one second duration '(see Max bytes/sec written to files (over 1 sec interval)', above). If the test is only run for one second this number will be the same as 'Avg bytes/sec written to files'</td></tr>
<tr><td>Avg bytes/sec written to files</td><td>Average write bandwidth over duration of test.</td></tr>
<tr><td>Max latency of write (ns)</td><td>The longest time (in nanoseconds) to complete a write during the test. The number of the write is indicated in brackets.</td></tr>
<tr><td>Min bytes/sec (slowest write)</td><td>The theoretical minimum bandwidth, if every write had the maximum latency indicated above</td></tr>
<tr><td>Min latency of write (ns)</td><td>The shortest time (in nanoseconds) to complete a write during the test. The number of the write is indicated in brackets.</td></tr>
<tr><td>Max bytes/sec (fastest write)</td><td>The theoretical maximum bandwidth, if every write had the minimum latency indicated above</td></tr>
<tr><td>Avg latency of write (ns)</td><td>The average time (in nanoseconds) to complete a write during the test.</td></tr>
</table>

## Multiple Queue Managers
This tool has now been extended to simulate multiple Queue Managers writing to the same IO device. 
Use the `--qm=2` option to run with 2 queue managers, each owning their own set of log files as specified by the configuration provided, noting that for qm 2 to 10, a numeric value is appended to the directory location. 

i.e If `--qm=3 --dir=/tmp/mqldt` is provided, directories labeled `/tmp/mqldt1, /tmp/mqldt2 and /tmp/mqldt3` need to be created before the test is run

Support for up to 10 QM is provided, although only a single block size can be executed per test.

## Container image (docker/podman)
There is a repo that builds a dockerized version of mqldt called [mqldt-c](https://github.com/ibm-messaging/mqldt-c).

The built image from that repo is available [here](https://quay.io/stmassey/mqldt) and can be pulled with the following command: 
```
docker pull quay.io/stmassey/mqldt
```
