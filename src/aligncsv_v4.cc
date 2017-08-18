// Filename: aligncsv.cc
// Purpose: align multiple csv files produced by Chromatof
// Author: Charles Peterson, Texas Biomed, August 2017
// Usage: aligncsv [-1] [-d <diff>] [-o <outfile>] [-m] [<filename>]+
//        -1 means force one line header on output (not required if
//           there is only one header anyway)
//        -d <diff> is floating point fraction < 1 (proportion) or integer
//           (arithmetic difference) by which 1st dimension time can vary in
//           the same record (defaults to 0.01 meaning 1%), only first time
//           in any record is checked
//        -o <outfile> write to this file instead of aligncsv.csv
//        -m Use "microsoft" excel formatting with trailing comma
//        -r Restrict output to chemical and time found in all files
//
// Output: aligncsv.csv file is written to working directory.
//  
// Notes: Input file(s) may have 2 headers, or one header
//
// If the first header has blank columns, a second header is required.
// A first header consists of qualifiers to the second header fields
//   and the first header name repeats for all blank columns until
//   the next nonblank name appears.
//
// Default is to output header(s) in same form as input.
//
// Optionally, the two header form can be changed to a one header form
//   for compatibility with other software by specifying option -1.  
//   In this case, the first header fields become suffixes to the
//   second header fields, using the HEADER_SEPARATOR defined below as @
//
// Files are joined horizontally, based on matching entries in the first
//   column ("peak", which identifies the peptide chemical), and matching
//   the first dimension time within the diff value.
//
// The peak column is written ONLY to the first column in the output file,
//   and not repeated for each of the subsequent file(s) which are being
//   joined.
//
// When some files are missing particular "peak" values, their columns have
//   no (aka null) entries, with commas separating them as always.
//
// Microsoft-excel-style trailing commas after the last valid field are removed
//    unless the -m option is specified.
//
// When run on linux, this program will terminate lines as Unix does, with \n.
// If this program is compiled on Windows, it should terminate lines like Windows.
//
// When the file is copied to the other platform, generally the system will do
//   the required conversion.
//
// Double quoted fields are written double quoted to the output file as well.
//-


#define MAXFILES 1000         // may be increased to system limits
#define HEADER_SEPARATOR "@"  // this must not be used in column names
#define UNIX_TERMINATOR "\n"

// #define MICROSOFT_TERMINATOR ",\r\n"
// this doesn't work.  standard library elides \r on linux systems
// and adds it on Windows systems

#define MICROSOFT_TERMINATOR ",\n"


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
#include <cctype>
#include <algorithm>
#include <cmath>

// STDPRE defines the prefix needed to get C++11 functionality
// TR1 is needed if compiler is pre C++11 (e.g. gcc 4.4.7)
// Comment this out for C++11 compliant compilers
// tr1 update might be required for unordered_map

//#define TR1 1

#ifdef TR1
#define STDPRE std::tr1
#include <tr1/unordered_map>
#else
#define STDPRE std
#include <unordered_map>
#endif

using std::ofstream;

STDPRE::unordered_map<std::string, std::string>Table;  // all field data
std::set<std::string>Chemicals; // just the chemical names
std::vector<std::string> Header;
std::vector<std::string> Header1;
std::vector<std::string> Header2;
std::vector<std::string> Filenames;
std::vector<int> DataColumns;

// A single input record
class ChemRecord {
public:
    std::vector<std::string> fields;  // data fields following chemical
    float time1;
    float time2;
    static bool higher (ChemRecord c1, ChemRecord c2) 
	{return c1.time1 > c2.time1;}
    int nfields () {return fields.size();}
};

ChemRecord NA;

// All the records in one file
STDPRE::unordered_map<std::string,std::vector<ChemRecord> > FileData;

// All the records in all the files
std::vector<STDPRE::unordered_map<std::string,std::vector<ChemRecord> > >AllFileData;

// A single output line (including data from all files)
class OutputRecord {
public:
    OutputRecord (std::string inputline, float intime1)
	{line=inputline;time1=intime1;}
    std::string line;
    float time1;
    static bool lower (OutputRecord r1, OutputRecord r2)
	{return r1.time1 < r2.time1;}
};

std::vector<OutputRecord> OutputLines;



// **** MAIN PROGRAM BEGINS HERE //

int main (int argc, char** argv)
{
    std::string LineTerminator = UNIX_TERMINATOR;
    float adiff = 0.01;
    bool afraction = true;
    std::ifstream infile[MAXFILES];  // std::vector not possible for ifstream
    int ninfiles = 0;
    std::string outname = "aligncsv.csv";
    int single_header = 0;
    bool restricted = false;

// parse arguments and open files

    if (argc < 2) {
	std::cout << "Usage: aligncsv [-1] [-d <diff>] [<filename>]+\n";
	std::cout << "-1 means force two headers to one\n";
	std::cout << "-d <diff> sets maximum alignment difference, default is .01 for 1%\n";
	std::cout << "   >1 will set integer difference, 0 means must be exactly same\n";
	std::cout << "-o <outfile> means output to this file (default is aligncsv.csv)\n";
	std::cout << "-m meaus use trailing comma format like Microsoft does\n";
	std::cout << "-r means restrict to chemical/times found in all files\n";
	return 0;
    }

    int iarg = 1;
    int starg = 0;
    while (iarg > starg)
    {
	starg = iarg;
    
	if (!strcmp(argv[iarg],"-1")) {
	    single_header = 1;
	    iarg++;
	}
	if (!strcmp(argv[iarg],"-d")) {
	    iarg++;
	    if (argc < 3) {
		std::cerr << "-d requires <diff> specification\n";
		return -1;
	    }
	    char* ppend;
	    adiff = strtof (argv[iarg],&ppend);
	    if (adiff < 0 || *ppend != 0) {
		std::cerr << "<diff> specification must be >= 0\n";
		return -1;
	    }
	    if (adiff >= 1) {
		afraction = false;
	    }
	    iarg++;
	}
	if (!strcmp(argv[iarg],"-o")) {
	    iarg++;
	    if (argc < 3) {
		std::cerr << "-o requires <outfilename> specification\n";
		return -1;
	    }
	    if (FILE *testfile = fopen (argv[iarg],"r")) {
		fclose (testfile);
		std::cerr << "file named " << argv[iarg] << 
		    " already exists and must be deleted first\n";
		return -1;
	    }
	    outname = argv[iarg];
	    iarg++;
	}
	if (!strcmp(argv[iarg],"-m")) {
	    LineTerminator = MICROSOFT_TERMINATOR;
	    iarg++;
	}
	if (!strcmp(argv[iarg],"-r")) {
	    restricted = true;
	    iarg++;
	}
    }

    std::ofstream outfile;
    outfile.open(outname.c_str());
    if (outfile.fail())
    {
	std::cerr << "Unable to open output file\n";
	return -10;
    }

    int first_file_index = iarg;

    for (; iarg < argc; iarg++)
    {
	if (ninfiles >= MAXFILES)
	{
	    std::cerr << "Maximum " << MAXFILES <<
		" number of files exceeded";
	    return -1;
	}
	int ifile = iarg - first_file_index;
	infile[ifile].open (argv[iarg]);
	if (infile[ifile].fail())
	{
	    std::cerr << "No Such File: " << argv[iarg] << "\n";
	    return -1;
	}
	if (!infile[ifile].is_open()) {
	    std::cerr << "File " << argv[iarg] << " was not opened\n";
	    return -1;
	}
	ninfiles++;
	Filenames.push_back(argv[iarg]);
	DataColumns.push_back(0);
    }

// Read In Files

    std::string aline;
    std::string field;
    std::string last_field = "";
    int count_empties;
    int record_size;
    bool header2_required;

    std::vector<std::vector<std::string> > Lines_in_file;
    for (int ifile = 0; ifile < ninfiles; ifile++)
    {
	FileData.clear();
	std::vector<std::string> header1;
	std::vector<std::string> header2;
	header2_required = false;

	std::cout << "\nReading file " << Filenames[ifile] << "\n";

// Current design permits (but does not require) two headers
//   First header is incomplete if there are nulls so second is then read
//   Composite field names are constructed using both header values with
//   HEADER_SEPARATOR in between.  The first header value, if null, is
//   taken from the previously defined name.

// CSV files created by Microsoft and similar programs are created with
//   a non-standard trailing
//   comma after the last non-null field (in effect, using comma as cell
//   terminator rather than cell separator).  This is allowed for but not
//   required (making this more complicated than I would like).

//   read first header

	getline (infile[ifile], aline);
//	std::cout << "Got line1: " << aline << "\n";

	std::stringstream sstream1(aline);
	count_empties = 0;
	bool last_field_empty = false;
	bool last_empty_field_counted = false;
	while(std::getline (sstream1, field, ',') )
	{
	    last_field_empty = false;
	    last_empty_field_counted = false;
//	    printf ("Got name: %s\n",field.c_str());
	    if (field.empty())
	    {
		count_empties++;
		field = last_field;
		last_field_empty = true;
		last_empty_field_counted = true;
	    } else {
// handle non-standard line terminators as uncounted empty
		bool empty_field = false;
		int flen = field.length();
		if (flen < 3) {
		    empty_field = true;
		    for (int fin = 0; fin < flen; fin++)
		    {
			if (!std::isspace(field[fin]))
			{
			    empty_field = false;
			}
		    }
		    if (empty_field)
		    {
//			std::cout << "...virtually empty\n";
			field = last_field;
			last_field_empty = true;
		    }
		}
	    }
	    header1.push_back (field);
	}

	if (last_field_empty)
	{
//	    std::cout << "last field empty so popping\n";
	    header1.pop_back();
	}
	if (last_empty_field_counted)
	{
	    count_empties--;
	}

	if (count_empties)
	{
	    header2_required = true;
	}

//   read second header if required

	if (header2_required)
	{
	    count_empties = 0;
	    getline (infile[ifile], aline);
//	    printf ("Got line2: %s\n",aline.c_str());
	    std::stringstream sstream2(aline);
	    last_field_empty = false;
	    last_empty_field_counted = false;
	    while(std::getline (sstream2, field, ',') )
	    {
		last_field_empty = false;
		last_empty_field_counted = false;
//		printf ("Got name: %s\n",field.c_str());
		if (field.empty())
		{
//		    std::cerr << "empty field in second header, file " <<
//			Filenames[ifile];
		    count_empties++;
		    last_field_empty = true;
		    last_empty_field_counted = true;
		} else {
// handle non-standard line terminators as uncounted empty
		    bool empty_field = false;
		    int flen = field.length();
		    if (flen < 3) {
			empty_field = true;
			for (int fin = 0; fin < flen; fin++)
			{
			    if (!std::isspace(field[fin]))
			    {
				empty_field = false;
			    }
			}
			if (empty_field)
			{
//			    std::cout << "...virtually empty\n";
			    field = last_field;
			    last_field_empty = true;
			}
		    }
		}
		header2.push_back (field);
	    }
	    if (last_field_empty)
	    {
//		std::cout << "last field empty so popping\n";
		header2.pop_back();
	    }
	    if (last_empty_field_counted)
	    {
		count_empties--;
	    }
	    if (count_empties)
	    {
		std::cerr << "Second header has incomplete fields in file: "
			  << Filenames[ifile] << "\n";
		return -2;
	    }
	    if (header2.size() != header1.size())
	    {
		std::cerr << "First and second headers different size\n";
		return -3;
	    }
	    record_size = header2.size();
	    std::cout << "Two headers read successfully.\n";
	} else {
	    std::cout << "One header read successfully.\n";
	}

// Create composite field names from both headers
// Second header line becomes "suffix" (e.g. "@subject-1")
//   If second header field is blank, the preceding non-blank, if any, is used
// If quotes are present in either name, they apply to both but are removed
//   in between.

	std::vector<std::string> fields;
	if (!header2_required)
	{
	    fields = header1;
	}
	else
	{
	    std::string last_suffix = "";
	    for (int ich = 0; ich < record_size; ich++)
	    {
		bool quote_prefix = false;
		bool quote_suffix = false; 
		std::string composite = header2[ich];
		if ('"' == composite[composite.length()-1]) {
		    composite.erase(composite.length()-1);
		    quote_prefix = true;
		}

		std::string suffix = header1[ich];
		if (suffix.length() > 0) {
		    last_suffix = suffix;
		} else {
		    if (last_suffix.length() > 0) {
			suffix = last_suffix;
		    } else {
			suffix = "";
		    }
		}
		if (suffix[0] == '"' ) {
		    if (single_header) {
			suffix.erase(0,1);
		    }
		    quote_suffix = true;
		}
		if (suffix.length() > 0) {
		    composite += HEADER_SEPARATOR;
		    composite += suffix;
		}
		if (quote_prefix && composite[composite.length()-1] != '"') {
		    composite += "\"";
		}
		if (quote_suffix && !quote_prefix) {
		    composite = "\"" + composite;
		}
		fields.push_back (composite);
		Header.push_back (composite);
		if (ifile==0 || ich > 0) {
		    Header1.push_back (suffix);
		    Header2.push_back (header2[ich]);
		}
		DataColumns[ifile]++;
	    }
	}
	DataColumns[ifile]--;  // Remove peak column


// ORIGINAL VERSION did this:
// Read records into hashtable using composite names:
//   Each hastable entry is a string (field value) and the key
//     is <chemical_name>+<composite-field-name> because
//     the chemical name applies to each row and the field name applies to
//     the column within each row

// REVISION 2 now does this:
//   Original design was assuming only one record per chemical.  But
//     a chemical may appear in multiple records in one file, and those
//     records are distinguished by different values in the first and/or
//     second retention time dimensions.  Joining is really understood as
//     alighning related records, which have the same chemical AND
//     similar first and second weight times.

//   The new algorithm saves each LINE of fields by chemical name & sequential
//   index number.  Information is saved this way separately for each file.
//   and at the same time, a hash of chemical names is also created.
//
//   For output, we cycle through each chemical name.
//     For each file having that chemical, we get the first retention
//     time(s) available in records in that file (as an average for each
//     applicable record).  Starting from the lowest
//     retention time found in all files, we try to find matching lines in
//     other files within 10%.  Once we have set of matching lines from all
//     possible files, we start from the longest retention time in that
//     group, and see if it better matches the next higher retention time(s)
//     we would find, and thereby peel away the higher ones that that
//     have better matches above, until the current line matches best.
//
//   The same selection method is applied to the second retention times.
//     If the set of records found for second retention times is different,
//     a warning message is given.
//
//  The complete set of matching lines is written to the output file and
//     removed from the working set, and the algorithm continues until all
//     chemicals have been processed.

//  Each field in line of each file is stored as a std::vector of std::string
//      (except the first chemical name field)
//  That is then stored in a unordered_map using file-index, chemical-name,
//    and record index number
//

	std::vector<std::string>* Fields;

	while (getline(infile[ifile], aline))
	{
//	    std::cout << "Got line " << aline << "\n";

// requires explicit parsing because there may be quoted fields
//   first, get first field, chemicalName

	    std::vector<std::string> Fields;

	    int column = 0;
	    std::string chemicalName;

	    std::string::iterator it = aline.begin();
	    bool finis = false;
	    unsigned quotes = 0;
	    char prev = 0;
	    while ( !finis && it != aline.end() )
	    {
		switch (*it) {
		case '"':
		    ++quotes;
		    break;
		case ',':
		    if (quotes == 0 || (prev == '"' && (quotes & 1) == 0)) {
			finis = true;
		    }
		    break;
		default:;
		}
		if (!finis) {
		    chemicalName += prev = *it;
		}
		it++;
	    }
	    Chemicals.insert (chemicalName);

//   next, get each field and add to table for this chemicalName and column

	    std::string field;
	    std::string key;
	    while (1) {
		finis = false;
		quotes = 0;
		field = "";
		while ( !finis && it != aline.end() )
		{
		    bool cr = false;
		    switch (*it) {
		    case '"':
			++quotes;
			break;
		    case ',':
			if (quotes == 0 || (prev == '"' && (quotes & 1)==0)) {
			    finis = true;
			}
			break;
		    case '\r':
			cr = true;
			break;
		    default:;
		    }
		    if (!finis && !cr) {
			field += prev = *it;
		    }
		    it++;
		}
		Fields.push_back(field);
		column++;
		if (it == aline.end() ) {
		    break;
		}
	    }
	    ChemRecord chemrecord;
	    chemrecord.fields = Fields;
	    float stime = 0;

// stof not supported in gcc 4.4.7
//	    std::string::size_type sz;
//	    stime = std::stof (Fields[1], &sz);

// instead using strtof, and skip past quotes if used
	    const int bufsiz = 128;
	    char pstring[bufsiz];
	    strncpy (pstring,Fields[1].c_str(),bufsiz);
	    char* ppstring;
	    if (pstring[0] == '"') {
		ppstring = &pstring[1];
	    } else {
		ppstring = &pstring[0];
	    }
	    char* ppend;
	    stime = strtof (ppstring, &ppend);
	    if (stime==0 || (*ppend != '\0' && *ppend != '"')) {
		std::cerr << "error reading time value: " << Fields[1] << "\n";
		return -1;
	    }
	    chemrecord.time1 = stime;
	    if (FileData.count(chemicalName)) {
		FileData[chemicalName].push_back(chemrecord);
	    } else {
		std::vector<ChemRecord> crecords;
		crecords.push_back(chemrecord);
		FileData[chemicalName] = crecords;
	    }
	    
	}
	if (infile[ifile].bad()) {
	    std::cerr << "error reading file\n";
	    return -1;
	}
	AllFileData.push_back(FileData);
    } // End reading all files
    std::cout << "Finished reading all files\n";


// write the output headers

    if (single_header || !header2_required) {
	for (int ifield = 0; ifield < Header1.size(); ifield++)
	{
	    if (ifield > 0) {
		outfile << ",";
	    }
	    outfile << Header[ifield];
	}
	outfile << LineTerminator;
    } else {
	std::string lastfield = "";
	for (int ifield = 0; ifield < Header1.size(); ifield++) {
	    if (ifield > 0) {
		outfile << ",";
	    }
	    if (Header1[ifield] != lastfield) {
		outfile << Header1[ifield];
	    }
	    lastfield = Header1[ifield];
	}
	outfile << LineTerminator;
	if (header2_required)
	{
	    for (int ifield = 0; ifield < Header2.size(); ifield++) {
		if (ifield > 0) {
		    outfile << ",";
		}
		outfile << Header2[ifield];
	    }
	    outfile << LineTerminator;
	}
    }

//              WRITE OUTPUT DATA

// iterate through each chemical seen

     int records_written = 0;
     std::cout << "Number of chemicals found: " << Chemicals.size() << "\n";
     for (std::set<std::string>::iterator 
	      it = Chemicals.begin(); it != Chemicals.end(); ++it)
     {
	 std::string keychem = *it;
	 bool more_data_seen = true;
	 while (more_data_seen) {
	     std::string outline = keychem;
	     
// Obtain first and second lowest retention time records from all files

	     std::vector<ChemRecord> lowest_recs;
	     float lowest_time1 = 0;
	     float second_lowest_time1 = 0;

	     int ifile;
	     more_data_seen = false;
	     for (ifile = 0; ifile < ninfiles; ifile++)
	     {
		 if (AllFileData[ifile].count(keychem) && 
		     AllFileData[ifile][keychem].size()) {
// sort in place (lowest to back)
		     std::sort (AllFileData[ifile][keychem].begin(),
				AllFileData[ifile][keychem].end(),
				ChemRecord::higher);

// pop the lowest
		     ChemRecord lowest  = AllFileData[ifile][keychem].back();
		     AllFileData[ifile][keychem].pop_back();
		     if (lowest_time1 == 0 ||
			 lowest_time1 > lowest.time1) {
			 lowest_time1 = lowest.time1;
		     }
		     lowest_recs.push_back(lowest);

// check the second lowest for time only (don't pop)

		     if (AllFileData[ifile][keychem].begin() ==
			 AllFileData[ifile][keychem].end() ) {
		     } else {
			 more_data_seen = true;
			 float test_lowest_time1 = 
			     AllFileData[ifile][keychem].back().time1;
			 if (second_lowest_time1 == 0 ||
			     second_lowest_time1 > test_lowest_time1) {
			     second_lowest_time1 = test_lowest_time1;
			 }
		     }
		 } else {
		     lowest_recs.push_back(NA);
		 }
	     }

// Now, for each record in our lowest_recs set
//    See if it is higher that adiff above the lowest
//    See if it is closer to the second_lowest than lowest
//      If either condition applies, push it back

//	     std::cout << "aligning\n";
	     for (ifile = 0; ifile < ninfiles; ifile++)
	     {
		 ChemRecord test_record = lowest_recs[ifile];
		 float cutoff;
		 if (afraction) {
		     cutoff = (1 + adiff) * lowest_time1;
		 } else {
		     cutoff = lowest_time1 + adiff;
		 }
		 bool pushback = false;
		 if (test_record.nfields() != 0) {
		     if (test_record.time1 > cutoff) {
			 pushback = true;
		     } else if (test_record.time1 - lowest_time1 > 
				std::abs(second_lowest_time1 - test_record.time1))
		     {
			 pushback = true;
		     }
		     if (pushback) {
//			 std::cout << "doing pushback on record with time " <<
//			     test_record.time1 << "\n";
			 more_data_seen = true;
			 AllFileData[ifile][keychem].push_back(test_record);
			 lowest_recs[ifile] = NA;
		     }
		 }
	     }

// Accumulate all the lowest records that haven't been pushed back
// Write out blanks for records that don't exist or have been pushed back

//	     std::cout << "Accumulating\n";
	     bool unfound = false;
	     for (ifile=0; ifile < ninfiles; ifile++)
	     {
		 bool first_skipped = false;
		 if (lowest_recs[ifile].nfields()) {
		     std::vector<std::string>::iterator field = 
			 lowest_recs[ifile].fields.begin();
		     std::string last_field;
		     int column = 0;
		     for (; field != lowest_recs[ifile].fields.end();field++)
		     {
// data records may have extra terminating comma (microsoft nonstandard csv)
// only fields with names are valid
			 if (++column > DataColumns[ifile]) {
			     break;
			 }

			 outline += ",";
			 outline += *field;
			 last_field = *field;
		     }
		 } else {
// output empty fields for this file
		     unfound = true;
		     int column = 0;
		     while (++column <= DataColumns[ifile])
		     {
			 outline += ",";
		     }
		 }
	     }
	     outline += LineTerminator;
	     if (!unfound || !restricted) {
		 OutputLines.push_back (OutputRecord(outline,lowest_time1));
	     }
	 }
     }

// Sort all output records by time1

     std::sort (OutputLines.begin(),OutputLines.end(),OutputRecord::lower);

// Write all output records

     std::vector<OutputRecord>::iterator it;
     for (it = OutputLines.begin(); it != OutputLines.end(); it++)
     {
	 outfile << it->line;
	 records_written++;
     }

     std::cout << "\n" << records_written 
	       << " records written to "
	       << outname << "\n\n";
    return 0;
}

