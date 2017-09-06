# mqldt - IBM MQ Log Disk Tester

The purpose of this tool is to test the capability of a Linux mounted filesystem being used (or being proposed) to host an MQ transaction log.

![#f03c15](https://placehold.it/15/f03c15/000000?text=+)<b> Warning!!</b>  
<ul><li>If any queue manager is currently using the filesystem being tested:</li>
<ul><li>Do NOT specify existing log files to write to.</li>
<li>Create a new directory on the filesystem to be tested (this must be in the same file system as the current MQ logs)</li>
<li>Running this tool WILL adversely affect the performance of the existing queue manager(s), for the duration of the test, (which may, in turn, cause applications problems whose symptoms last beyond the filesystem test.</li>
</ul></ul>

The tool is similar in purpose to fio, but has been written to test writing to a filesystem with the same options, and in the same way, that MQ writes to its transaction logs, without having to specify lots of options on the command line to get it right.

There are additionally, some problems with the use of fio, which this tool addresses (like being unable to get the open and write options exactly the same as MQ, and the pre-loading and usage pattern of the log files).

What this tool cannot do, is tell you exactly what performance you will get from your persistent MQ workload, even when log writes are the limiting factor. The workload on a machine can impact the performance of the IOPs, especially on modern multi-core machines. The best solution is always to test MQ itself.

<pre>
Usage:

mqldt &ltoptions&gt

Options:
--dir         : Directory in which create and test files being written to. 
--filePrefix  : prefix of test files
--bsize       : Blocksize(s), of writes to be used. Can be a single value, or a commseperated list. Supports k suffix.
--fileSize    : File size of each test file
--numFiles    : Number of files to create and use
--duration    : Number of seconds to test
--csvFile     : Optional csv output file
</pre>
e.g.
mqldt --dir=/var/san1/testdir --filePrefix=mqtestfile --bsize=5K,9K,17K,24K,49K,48K,54K,62K,77K,95K,105K --fileSize=67108864 --numFiles=16 --duration=40 --csvFile=./san_log.csv

Sample output:

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

Total writes to files          :          46,790
Total bytes written to files   :   6,132,858,880
Max bytes/sec written to files :     311,689,216
Min bytes/sec written to files :     299,630,592
Avg bytes/sec written to files :     306,754,931

Max latency of write (ns)      :       4,669,311
Min latency of write (ns)      :         375,204
Avg latency of write (ns)      :         415,704
</pre>

Use the values in qm.ini (or those planned) to calculate some of the parameters to MQLDT.

<table>
<tr><td>MQLDT Parm</td><td>qm.ini parm(s)</td></tr>
<tr><td>--dir</td><td>Any directory hosted by the same file-system as <i>Log:LogPath</i></td></tr>
<tr><td>--fileSize</td><td><i>Log:LogFilePages</i> x 4096</td></tr>
<tr><td>--numFiles</td><td><i>Log:LogPrimaryFiles</i></td></tr>
</table>

