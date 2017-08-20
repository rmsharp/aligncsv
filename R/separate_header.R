#' Separate header lines from data lines.
#' 
#' Takes tokenized lines, separates header lines from data lines, trims off
#' trailing empty tokens, and packages the header lines and the data lines
#' into a list with two parts: header and data.
#' 
#' @param lines dataframe of character vectors where the tokens were created by 
#' read.csv(). 
#' Each line of the original file is tokenized into fields defined by commas 
#' separated values.
#' @param removed_trailers logical vector of length one indication whether or
#' not to remove Microsoft-excel-style trailing commas after the last valid 
#' field. 
#' This is set by to TRUE by default unless the -m option is specified.
#' @import stringi
#' @export
separate_header <- function(lines, remove_trailers = TRUE) {
  ptr <- 1
  count_empties <- length(lines[ptr, ][!is.na(lines[ptr, ]) & 
                                         lines[ptr, ] == ""])
  header <- list(header1 = character(0), header2 = character(0))
  header[[ptr]] <- as.character(lines[ptr, ])
  if (count_empties > 0) { # Need another line for the header
    ptr <- ptr + 1
    header[[ptr]] <- as.character(lines[ptr, ])
  }
  if (is.na(lines[ptr, ][length(lines[ptr, ])]) & remove_trailers) {
    for (i in 1:ptr) {
      header[[i]] <- header[[i]][1:(length(header[[i]]) - 1)]
    }
    lines <- lines[ , 1:(length(lines[i, ]) - 1)]
  }
  header <- header[1:ptr]
  ptr <- ptr + 1
  .df <- data.frame()
  connect <- ifelse(header$header1 == "", "", "_")
  header1_2 <- stri_c(header$header1, connect, header$header2)
  
  if (nrow(lines) >= ptr) {
    .df <- lines[ptr:nrow(lines), ]
    names(.df) <- header1_2
    list(header = header, data = .df)
  } else {
    list(header = header, data = .df)
  }
} 
