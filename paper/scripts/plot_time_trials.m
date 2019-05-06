% Twister 3 time trials
% 2019.05.06

% Turn time (900 RPM, 40 fwd, 10 back)
turn_time = 3;

% dimitra
load_d = [27, 20, 28, 15, 12, 29];
turn_fuse_d = [15, 16, 28, 18, 15, 17];
cut_store_d = [20, 15, 11, 21, 16, 20];
total_d = load_d + turn_fuse_d + cut_store_d;

% marie
load_m = [24, 42, 28,  27, 45, 17]; %110,
turn_fuse_m = [12, 13, 17, 11, 11, 11]; %11
cut_store_m = [12, 12, 9, 8, 12, 12]; %9
total_m = load_m + turn_fuse_m + cut_store_m;

% jakob
load_j = [27, 15, 10, 10, 21, 12, 20, 9, 9, 17];
turn_fuse_j = [21, 21 ,22, 18, 22, 28, 24, 28, 26, 26];
cut_store_j = [10, 9, 8, 8, 9, 7, 5, 8, 8, 8];
total_j = load_j + turn_fuse_j + cut_store_j;

% combined
load = [load_d, load_m, load_j];
turn_fuse = [turn_fuse_d, turn_fuse_m, turn_fuse_j];
cut_store = [cut_store_d, cut_store_m, cut_store_j];
totals = load + turn_fuse + cut_store;

%% Figure
close all
figure('units', 'centimeters', 'position', [2, 2, 10, 10])
hold all
a = 0.3;

plot([0, 0 + a], [mean(totals) mean(totals)], 'k-', 'linewidth', 2)
plot(0 + a * rand(size(total_d)), total_d, 'k^')
plot(0 + a * rand(size(total_m)), total_m, 'ko')
plot(0 + a * rand(size(total_j)), total_j, 'kx')

plot([1, 1 + a], [mean(load) mean(load)], 'k-', 'linewidth', 2)
plot(1 + a * rand(size(load_d)), load_d, 'k^')
plot(1 + a * rand(size(load_m)), load_m, 'ko')
plot(1 + a * rand(size(load_j)), load_j, 'kx')

plot([2, 2 + a], [mean(turn_fuse) mean(turn_fuse)], 'k-', 'linewidth', 2)
plot(2 + a * rand(size(turn_fuse_d)), turn_fuse_d, 'k^')
plot(2 + a * rand(size(turn_fuse_m)), turn_fuse_m, 'ko')
plot(2 + a * rand(size(turn_fuse_j)), turn_fuse_j, 'kx')

plot([3, 3 + a], [mean(cut_store) mean(cut_store)], 'k-', 'linewidth', 2)
plot(3 + a * rand(size(cut_store_d)), cut_store_d, 'k^')
plot(3 + a * rand(size(cut_store_m)), cut_store_m, 'ko')
plot(3 + a * rand(size(cut_store_j)), cut_store_j, 'kx')

xlim([-0.5, 3.5])
ylim([0 80])
set(gca, 'tickdir', 'out')

export_fig('tt-time-trials.pdf','-pdf','-transparent', gcf)
