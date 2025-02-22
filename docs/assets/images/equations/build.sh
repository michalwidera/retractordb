# https://www.reddit.com/r/LaTeX/comments/px5gzl/latex_equation_to_svg_tex2svg/

for x in equations/*.tex; do 

    echo $x

    pdflatex --interaction=batchmode -jobname temp/latex.temp $x > /dev/null
    pdfcrop --margins 1 temp/latex.temp.pdf temp/crop.pdf > /dev/null
    pdf2svg temp/crop.pdf output/$(basename --suffix=.tex -- $x).svg  > /dev/null

done   