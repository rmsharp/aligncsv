#' Get the settings from the command line arguments
#' This function sets up the parameters to use by the script based on default 
#' values and command line arguments
#' @import stringi
get_settings <- function() {
  UNIX_TERMINATOR <- "\n"
  MICROSOFT_TERMINATOR <- ",\n"
  ## MICROSOFT_TERMINATOR <- ",\r\n"
  ## this doesn't work.  standard library elides \r on linux systems
  ## and adds it on Windows systems
  
  args <- commandArgs(trailingOnly = TRUE)
  cat(args, sep = "\n")
  if (length(args) < 2) {
    stop(stri_c(
      "Usage: aligncsv [-1] [-d <diff>] [<filename>]+\n",
      "  -1 means force two headers to one\n",
      "  -d <diff> sets maximum alignment difference, default is 
                    0.01 for 1%\n",
      "      >1 will set integer difference, 0 means must be exactly same\n",
      "-o <outfile> means output to this file (default is aligncsv.csv)\n",
      "-m means use trailing comma format like Microsoft does\n",
      "-r means restrict to chemical and times found in all files\n"))
  }
  if (any(tolower(args) == "-m")) {
    LineTerminator = MICROSOFT_TERMINATOR
    args <- args[tolower(args) != "-m"]
  } else {
    LineTerminator = UNIX_TERMINATOR
  }
  if (any(tolower(args) == "-r")) {
    restrict <- TRUE
    args <- args[tolower(args) != "-r"]
  } else {
    restrict <- FALSE
  }
  if (any(tolower(args) == "-o")) {
    extracted_args <- extract_command_arg(args, "-o")
    args <- extracted_args$args
    if (args$found == TRUE) {
      output_file <- args$file
    } else {
      output_file <- ""
    }
    cat("output file: ", output_file, "\n")
  }
  i <- 1
  if (args[i] == "-1") {
    single_header = 1
    cat(stri_c("single_header = 1, i = :", i, "; args[i] = ", args[i], "\n"))
    i <- i + 1
  }
  if (args[i] == "-d") {
    cat(stri_c("args[i] == \"-d\":", " i = :", i, "; arge[i] = ", args[i], "\n"))
    i <- i + 1
    cat(stri_c("length(args) < 3: ", length(args) < 3, "; args[i] = ", args[i], "\n"))
    if (length(args) < 3) {
      stop("-d is missing <diff>, which must be >= 0\n")
    } else {
      diff <- as.numeric(args[i])
      if (diff < 0) {
        stop("<diff> specification must be >= 0\n")
      } else if (diff >= 1) {
        diff_is_fraction <- FALSE
      } else {
        diff_is_fraction <- TRUE
      }
    }
    i <- i + 1
  }
  filenames <- args[i:length(args)]
  list(diff = diff, diff_is_fraction = diff_is_fraction, 
       LineTerminator = LineTerminator, 
       output_file = output_file, filenames = filenames)
}
