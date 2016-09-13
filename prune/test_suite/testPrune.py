import unittest
import os
import fnmatch
import subprocess
import filecmp


# find all files, output names
def find_fastq_files(directory, pattern):
    """Walks the directory structure, appending filenames to an array"""
    filenames = []
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                filenames.append(filename)
    filenames.sort()
    return filenames


# basic call to the shell application we are testing
def sub_process(command):
    return subprocess.check_output(
        command, stderr=subprocess.STDOUT, shell=True)


def file_compare(command, expected, returned):
    # testing expected file output
    subprocess.check_output(command, stderr=subprocess.STDOUT, shell=True)
    return filecmp.cmp(expected, returned)


def parse_fastq(filename):
    # output the file to a dictionary
    with open(filename) as f:
        lines = f.readlines()
    read = [item[:-1] for item in lines[1::4]]
    qual = [item[:-1] for item in lines[3::4]]
    return dict(zip(read, qual))


class TestCase(unittest.TestCase):

    def setUp(self):
        """Setup any thing used in muliple tests"""
        myR1file = " -1 fastqFiles/prune_R1.fastq "
        myR2file = " -2 fastqFiles/prune_R2.fastq "
        additFlags = "-N -F"
        myShellCmd = "../prune"
        self.myCommand = myShellCmd + myR1file + myR2file + additFlags

    def test_find_fastq_files_recursively(self):
        """Should return all fastq files from the sub directories"""
        self.assertEqual(find_fastq_files('fastqFiles', '*.fastq'),
                         ['fastqFiles/prune_R1.fastq',
                          'fastqFiles/prune_R2.fastq'])

    # copy this def to make new command tests
    def test_basic_input(self):
        """Should return that basic input works"""
        self.assertIn("2500", sub_process(self.myCommand))

    def test_specific_output(self):
        """Should return a tab delimited out put"""
        myExpectedOutput = ("PE_Reads\tPE_Kept\tAvg_PE_Left_Trim\t"
                            "Avg_PE_Right_Trim\tR1_Discarded\tR2_Discarded\t"
                            "SE_Reads\tSE_Kept\tAvg_SE_Left_Trim\t"
                            "Avg_SE_Right_Trim\tRun_Time\n2500.000000\t"
                            "2500.000000\t65.71\t65.71\t0.000000\t0.000000\t"
                            "0.000000\t0.000000\tnan\tnan\t0\n")

        self.assertEqual(sub_process(self.myCommand), myExpectedOutput)

    def test_file_compare(self):
        """Should  return that two files match line for line"""
        myR1file = " -1 fastqFiles/prune_R1.fastq "
        myR2file = " -2 fastqFiles/prune_R2.fastq "
        additFlags = "-F"
        myShellCmd = "../prune"
        myCommand = myShellCmd + myR1file + myR2file + additFlags
        myExpectedFile = "expected_R1.fastq"
        myReturnedFile = "non_R1.fastq"
        self.assertTrue(file_compare(
            myCommand, myExpectedFile, myReturnedFile))


if __name__ == '__main__':
    unittest.main()
