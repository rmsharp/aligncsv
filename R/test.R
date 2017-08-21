library(stringi)
library(readr)
test <- get_settings()
lines <- read.csv(file = "../data/lowest3.csv", header = FALSE, 
                  stringsAsFactors = FALSE)
head_and_data <- separate_header(lines, remove_trailers = TRUE)
