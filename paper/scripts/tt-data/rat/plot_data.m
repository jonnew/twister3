%% 
load('MUA4Jon.mat')
load('LFP4Jon.mat')

%% Spikes

close all
f = figure('units', 'centimeters','position', [5 5 10 10]);
ms = 2;
lim = 0.001;
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
export_fig('rat-ca1-units.png','-png','-transparent', '-r1000', gcf)

%% LFP

close all
f = figure('units', 'centimeters','position', [5 5 10 10]);
a = axes(f, 'units', 'centimeters','Position',[1 3.8 1.6 *3 + 0.4 1.6]);

hold all
plot(lfp.timestamp, lfp.data, 'k-')
set(gca, 'TickDir', 'out')
xlim([1191.8 1192.9])
ylim([-1.1 0.5])

%%
export_fig('rat-ca1-lfp.pdf','-pdf','-transparent', gcf)