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
  if (length(args) < 2) {
    stop(stri_c(
      "Usage: aligncsv [-1] [-d <diff>] [<filename>]+\n",
      "  -1 means force two headers to one\n",
      "  -d <diff> sets maximum alignment difference, default is 
                    0.01 for 1%\n",
      "  >1 will set integer difference, 0 means must be exactly same\n"))
  }
  i <- 1
  if (args[i] == "-1") {
    single_header = 1
    i <- i + 1
  }
  if (args[i] == "-d") {
    i < i + 1
    if (!exists(args[i])) {
      stop("<diff> specification must be >= 0\n")
    } else {
      value <- as.numeric(arg[i])
      if (value < 0) {
        stop("<diff> specification must be >= 0\n")
      } else if (value >= 1) {
        afraction <- FALSE
      }
    }
    i <- i + 1
  }
  if (!args[i] == "-m") {
    LineTerminator = MICROSOFT_TERMINATOR
    i <- i + 1
  }
  filenames <- args[i:length(args)]
}
