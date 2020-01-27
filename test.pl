#! /usr/bin/perl -w
my(@a);
while (defined($_= <>)) {
   my($hash, $label)= /^(\S*) (.*)$/;
   push @a, {hash=> $hash, label=> $label};
}
my $first= 1;
my(@m, $dist, $k);
foreach my $i (@a) {
   if ($first) {$first= 0;} else {print "\n";}
   print "Best matches for '$i->{label}':\n";
   @m= ();
   foreach my $j (@a) {
      next if $i eq $j || $i->{hash} eq '' || $j->{hash} eq '';
      $dist= 0;
      for ($k= length($i->{hash}); $k--; ) {
         my $hi= substr($i->{hash}, $k, 1) + 0;
         my $hj= substr($j->{hash}, $k, 1) + 0;
         $dist+= $hi ^ $hj;
      }
      push @m, {dist=> $dist, label=> $j->{label}};
   }
   foreach my $d (sort {$a->{dist} <=> $b->{dist}} @m) {
      print "\t$d->{dist}: $d->{label}\n";
   }
}
