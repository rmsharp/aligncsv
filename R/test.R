library("stringi")
library("readr")
source("get_settings.R")
source("separate_header.R")
source("extract_command_arg.R")
test <- get_settings()
cat(dput(test, file = ""))
# lines <- read.csv(file = "../data/lowest3.csv", header = FALSE, 
#                   stringsAsFactors = FALSE)
path <- "../data/"
lines <- read.csv(file = stri_c(path, test$filenames[1]), header = FALSE, 
                  stringsAsFactors = FALSE)
head_and_data <- separate_header(lines, remove_trailers = TRUE)
cat(names(head_and_data$data))
