%% 
load('tt6_spikes.mat')
load('tt6_continuous_referenced.mat')

MUA.amp1 = -tt6.amp1;
MUA.amp2 = -tt6.amp2;
MUA.amp3 = -tt6.amp3;
MUA.amp4 = -tt6.amp4;
%% Spikes

close all
f = figure('units', 'centimeters','position', [5 5 10 10]);
ms = 2;
lim = 1000;
alp = 0.01;

a = axes(f, 'units', 'centimeters','Position',[1 3 2 2]);
s = scatter(a, MUA.amp1, MUA.amp2,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

a = axes(f, 'units', 'centimeters','Position',[3 3 2 2]);
s = scatter(a, MUA.amp1, MUA.amp3,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

a = axes(f, 'units', 'centimeters','Position',[5 3 2 2]);
s = scatter(a, MUA.amp1, MUA.amp4,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

a = axes(f, 'units', 'centimeters','Position',[1 1 2 2]);
s = scatter(a, MUA.amp2, MUA.amp3,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

a = axes(f, 'units', 'centimeters','Position',[3 1 2 2]);
s = scatter(a, MUA.amp2, MUA.amp4,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

a = axes(f, 'units', 'centimeters','Position',[5 1 2 2]);
s = scatter(a, MUA.amp3, MUA.amp4,ms, 'ko', 'filled');
set(gca, 'xtick', [], 'ytick', [], 'visible','off')
daspect([1,1,1])
axis([0, lim, 0, lim])
s.MarkerFaceAlpha = alp;

%%
export_fig('mouse-ca1-units.png','-png','-transparent', '-r1000', gcf)

%% LFP

close all
f = figure('units', 'centimeters','position', [5 5 10 10]);
a = axes(f, 'units', 'centimeters','Position',[1 3.8 1.6 *3 + 0.4 1.6]);

hold all
plot(cont6_60sec.timestamp, cont6_60sec.data + repmat([0 1000 2000 3000], size(cont6_60sec.data, 1), 1), 'k-')
set(gca, 'TickDir', 'out')
xlim([4507.2 4507.8])
ylim([-1000 4000])

%%
export_fig('mouse-ca1-lfp.pdf','-pdf','-transparent', gcf)
