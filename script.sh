

my $thisTest = 2;
my @topoNodes = (0, 1, 2, 3, 4, 5, 6, 7, 255);
my $numTopoNodes = @topoNodes;

for(my $ind=0; $ind<$numTopoNodes; $ind++)
{
      `./vec_router $topoNodes[$ind] test$thisTest.initcosts$topoNodes[$ind] log$topoNodes[$ind] >/dev/null 2>/dev/null &`;
}

sleep(5);

`./man 6 send 3 "this is just the example topo, ok"`;

sleep(1);

#(examine logs)
