# LogicInterpreter
LogicDesign Homework

testcases are in data directory
test.1.01.txt, its corresponding result is ref.1.01.txt


How to run the program?

make

./truthtable data/test.1.01.txt


Main idea
(1)Use 3 arrays to store INPUT, OUTPUT, and TEMPERATY variables separately. Each element has 3 fields: sign-key, value, and name-string.
(2)Build a hash table to index every variable.
(3)Do TopologySort to solve the dependence issue.
