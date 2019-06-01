l = [1 1.5 2 2.5 3 3.5 4 4.5 5 5.5 6]';
w = [1.5 2.8 5.07 6.5 7.75 9.7 11.14 12.9 15 16.3 18.3]';

f = w * 9.81;

figure
hold on
scatter(l, f, 'k')
xlim([0.5 6.5])
ylim([0 200])
set(gca, 'tickdir', 'out')


export_fig('leaf-spring-k.pdf','-pdf','-transparent', gcf)