extract_command_arg <- function(args, arg) {
  found <- FALSE
  for (i in seq_along(args)) {
    cat(stri_c("i = ", i, "; args[i] = '", args[i], "'\n"))
    if (tolower(args[i]) == arg) {
      if (length(args) > i) {
        found <- TRUE
        value <- args[i + 1]
        args <- args[!seq_along(args) %in% c(i, i + 1)]
        break
      } else {
        stop(stri_c("Argument: '", arg, "' must be followed by a file name.\n"))
      }
    }
  }
  if (found) {
    list(found = TRUE, file = value, args = args)
  } else {
    list(found = FALSE, args = args)
  }
}
