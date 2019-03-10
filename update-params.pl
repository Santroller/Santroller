open my $fh2, '<', 'src/config/config.h' or die "Cannot open: $!";
my $repl = "parameters {\n";
while ( my $line = <$fh2> ) {
    chomp;
    if ($line =~ /^\Q#define\E/) {
        my @fields = split ' ', $line;
        $repl=$repl."        string(name: '$fields[1]', defaultValue: '$fields[2]', description: '')\n"
    }
}
$repl=$repl."        string(name: 'F_CPU', defaultValue: '16000000', description: '')\n    }";
open my $fh, '+<', 'Jenkinsfile' or die "Can't open file $!";
my $var = do { local $/; <$fh> };
my $re = qr/(parameters \{[^}]+\}?)/;
$var =~ s/$re/$repl/g;
seek($fh, 0, 0);
print $fh $var;
close $fh;