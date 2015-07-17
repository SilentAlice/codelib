# set datafile separator ';'

set key right bottom
set xlabel "delta"
set ylabel "CDF"
set yrange [0:1]

set term png
set output "cdf.png"
plot "cum.txt" using 1:2 notitle with linespoints smooth cumulative

