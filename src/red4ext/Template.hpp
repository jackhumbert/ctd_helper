const auto CTD_HELPER_CSS = 
#include "style.css.frag"
;
const auto CTD_HELPER_HIGHLIGHT = 
#include "highlight.min.js.frag"
;
// const auto CTD_HELPER_LINE_NUMBERS = 
// #include "line-numbers.min.js.frag"
// ;

const auto CTD_HELPER_HEADER =
fmt::format(R"(<!DOCTYPE html>
<html>
<head>
<style>
{}
</style>
<script>
{}


hljs.highlightAll();
//hljs.initLineNumbersOnLoad();
</script>
)", CTD_HELPER_CSS, CTD_HELPER_HIGHLIGHT);//, CTD_HELPER_LINE_NUMBERS);
