package us.ssmy;

import java.io.IOException;
import java.util.*;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.util.*;

public class WordCount {

  public static class Map extends MapReduceBase implements Mapper<LongWritable, Text, Text, IntWritable> {
    //value 1 for mapping, in private, hadoop-friendly format
    private final static IntWritable one = new IntWritable(1);
    //value of a word for mapping, in private, hadoop-friendly format
    private Text word = new Text();

    public void map(LongWritable key, Text value, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
      String line = value.toString(); //convert one line at a time to a string
      //break line into words based on whitespace or given tokens
      StringTokenizer tokenizer = new StringTokenizer(line," \t\n\r\f?\";<>~`!@#&&*()_+=/\\:{}[]|,.");
      while (tokenizer.hasMoreTokens()) {
        //for each token (word,) output the map value <[word],1>
        word.set(tokenizer.nextToken());
        output.collect(word, one);
      }
    }
  }

  public static class Reduce extends MapReduceBase implements Reducer<Text, IntWritable, Text, IntWritable> {
    public void reduce(Text key, Iterator<IntWritable> values, OutputCollector<Text, IntWritable> output, Reporter reporter) throws IOException {
      int sum = 0;//total count of occurances of a word
      while (values.hasNext()) {
        //sum the 1 from each word together to get a total count per word
        sum += values.next().get();
      }
      //output the final word count, <[word],[count]>
      output.collect(key, new IntWritable(sum));
    }
  }

  public static void main(String[] args) throws Exception {
    //Hadoop configuration for this job
    JobConf conf = new JobConf(WordCount.class);
    //name of the job to be shown
    conf.setJobName("wordcount");

    //set types of the keys and values
    //Text class chosen here because it implements WritableComparable.
    //Needs to be comparable for the key
    conf.setOutputKeyClass(Text.class);
    //IntWritable class chosen here because it also implements WritableComparable.
    //Needs to be writable for the value
    conf.setOutputValueClass(IntWritable.class);

    //pull the number of maps and reduces as arguments
    conf.setNumMapTasks(Integer.parseInt(args[2]));
    conf.setNumReduceTasks(Integer.parseInt(args[3]));

    //set the classes Hadoop will call for mapping, combining, and reducing
    conf.setMapperClass(Map.class);
    conf.setCombinerClass(Reduce.class);
    conf.setReducerClass(Reduce.class);

    //tell hadoop to take the input as text and break it up one line at a time
    conf.setInputFormat(TextInputFormat.class);
    //tell hadoop to output the file as plain text
    conf.setOutputFormat(TextOutputFormat.class);

    //set the locations for the input file and output file in arguments
    FileInputFormat.setInputPaths(conf, new Path(args[0]));
    FileOutputFormat.setOutputPath(conf, new Path(args[1]));

    //finally, run the job
    JobClient.runJob(conf);
  }
}
