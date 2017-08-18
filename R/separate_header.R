lines <- readLines(con = "../data/lowest3.csv", skipNul = TRUE)
lines <- lines[!lines == ""]
#' @import stringi
#' @export
separate_header <- function(lines) {
  ptr <- 1
  tokens <- stri_trim_both(stri_split_fixed(lines[ptr], ",")[[1]])
  count_empties <- length(tokens[tokens == ""])
  header <- list(header1 = character(0), header2 = character(0))
  header[[ptr]] <- tokens
  if (count_empties > 0) { # Need another line for the header
    ptr <- ptr + 1
    tokens <- stri_trim_both(stri_split_fixed(lines[ptr], ",")[[1]])
    header[[ptr]] <- tokens
  }
  if (tokens[length(tokens)] == "") {
    for (i in 1:ptr) {
      header[[i]] <- header[[i]][1:(length(header[[i]]) - 1)]
    }
    tokenized_lines <- vector(mode = "list", length = length(lines) - ptr)
    for (i in seq_along(lines)) {
      tokenized_lines[i] <- stri_trim_both(stri_split_fixed(lines[ptr], ",")[[1]])
    }
  }
  header <- header[1:ptr]
  ptr <- ptr + 1
  if (length(lines) >= ptr) {
    list(header = header, lines = lines[ptr:length(lines)])
  } else {
    list(header = header, lines = character(0))
  }
} 
