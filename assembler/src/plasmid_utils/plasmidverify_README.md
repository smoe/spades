

# plasmidVerify: plasmid contig verification tool

plasmidVerify classifies contigs (output of metaplasmidSPAdes or other assemblers) as plasmidic or non-plasmidic, based on gene content. 


plasmidVerify predicts genes in the contig using Prodigal in the metagenomic mode, runs hmmsearch on the predicted proteins, 
and classifies the contig as plasmidic or chromosomal by applying the Naive Bayes classifier (NBC). 
For the set of predicted HMMs, plasmidVerify uses trained NBC to classify this set to be plasmidic or chromosomal. 


## Installation

### Requirements

plasmidVerify is a Python script, thus, installation is not required. However, it has following dependencies:

* Python 2.7+,
* Biopython,
* Prodigal,
* hmmsearch (from the hmmer package),
* Pfam-A database (check most recent version - http://pfam.xfam.org, FTP tab)

### Optional BLAST verification

You can verify your output by BLAST to check if you found novel plasmid. In this case you need to have blastn in your $PATH and provide path to local copy of nr database. 

## Usage 

    ./plasmidverify.py 
            -f Input fasta file
            -o output_directory 

            Optional arguments:
            -h, --help  Show the help message and exit
            -b          Run BLAST on input contigs
            -db DB      Path to BLAST db
            -hmm HMM    Path to Pfam-A HMM database


Output file input_file_result_table.csv contain comma-separated table with predicted HMMs and verification result.
