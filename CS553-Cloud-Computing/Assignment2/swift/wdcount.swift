type file;

app (file out) count(file script, file input)
{
 python @script filename(input) stdin=filename(input) stdout=filename(out);
}
file script <"countWords.py">;

file data[] <filesys_mapper; location="/mnt/my-data", prefix="data">;
file counts[] <simple_mapper;location="data", prefix="counts_", suffix=".out", padding=3>;

foreach f, i in data
{
 counts[i] = count(script, f);
}
