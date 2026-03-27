function [area_id] = ml_lc(rssi1, rssi2, rssi3)
    load('model.mat');
    feat = [rssi1; rssi2; rssi3];
    area_id = predictFcn(feat);
end