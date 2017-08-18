lines <- readLines(con = "../data/lowest3.csv", skipNul = TRUE)
tokenize_lines <- function(lines) {
  lines <- lines[!lines == ""]
  tokenized_lines <- vector(mode = "list", length = length(lines))
  for (i in seq_along(lines)) {
    tokenized_lines[[i]] <- 
      stri_trim_both(stri_split_fixed(lines[i], ",")[[1]])
  }
  tokenized_lines
}
#' @import stringi
#' @export
separate_header <- function(lines) {
  ptr <- 1
  count_empties <- length(lines[[ptr]][lines[[ptr]] == ""])
  header <- list(header1 = character(0), header2 = character(0))
  header[[ptr]] <- lines[[ptr]]
  if (count_empties > 0) { # Need another line for the header
    ptr <- ptr + 1
    header[[ptr]] <- lines[[ptr]]
  }
  if (lines[[ptr]][length(lines[[ptr]])] == "") {
    for (i in 1:ptr) {
      header[[i]] <- header[[i]][1:(length(header[[i]]) - 1)]
    }
    for (i in seq_along(lines)) {
      lines[[i]] <- lines[[i]][1:(length(lines[[i]]) - 1)]
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
