%% function csmaca
clear all; close all;

NumSta=[10:10:50];
NumIter=5;

SimuTime=1000;              % Simulation duration
SlotTime=1e-6;              % Duration of each timeslot
PktLen=180;                 % Number of bits per packet
DataRate=[6 9 12 18].*1e6;  % Available data rate (Mbps)
CWmin=32;                                       % Minimum contention window
CWmax=1024;                                     % Maximum contention window

for i=1:length(NumSta)
    for j=1:NumIter
        nn = NumSta(i);
        StaRate=DataRate(ceil(rand(1,nn)*4));       % Random rate of each station

        lambda=ones(1,nn)*10000;                    % Mean arrival of stations
        InterPktTime=(ones(1,nn)./lambda)/SlotTime; % Arrival time of the first packets
        StaPktCnt=zeros(1,nn);                      % Accumulated packet count of each station
        StaPktQ=[];                                 % Packet queue of each station
        SentBitCnt=zeros(1,nn);                     % Number of sent bits per station

        CW=ones(1,nn)*CWmin;                      % CW of each station
        % backoff=ones(1,nn).*CW;                     % Initial backoff counter
        backoff = randi([1,(CWmin)],1,nn);

        isBusy=0;           % flag denoting whether the medium is busy
        NumTx=0;            % Accumulated number of transmissions
        NumCollision=0;     % Accumulated number of collisions

        for t=1:SimuTime
            sprintf("=========== timeslot: %d ============", t)
            % schedule packet arrival events
            [InterPktTime, StaPktQ, StaPktCnt] = generate_pkt(InterPktTime, nn, PktLen, SlotTime, StaPktQ, StaPktCnt,lambda);

            % When the medium is idle
            if (~isBusy)
                % identify stations who have packets queued
                uid = find(StaPktCnt > 0);
                % continue if no one has packet
                if (length(uid) == 0)
                    continue;
                end
                % all pending stations count down together
                backoff(uid) = backoff(uid) - 1;

                % trigger transmissions when someone counts down to 0
                if (min(backoff(uid)) == 0)
                    % TODO: modify the following block to enable CSMA/CA
                    % identify the station ID of senders
                    ix = find(backoff(uid)==0);
                    uid = uid(ix);
                    winner = uid;
                    % winner_ix = find(rand(1,length(uid)) < 0.2/nn*10) % TODO: now pick by probability. Should be removed
                    
                    % TODO END

                    % check whether collisions occur
                    if(length(winner) == 1)
                        SentBitCnt(winner) = SentBitCnt(winner) + StaPktQ(winner);
                        StaPktCnt(winner) = StaPktCnt(winner)-1;
                        StaPktQ(winner,1:end-1) = StaPktQ(winner,2:end);
                        % TODO: update CW here
                        CW(uid) = CWmin;
%                         backoff(uid) = randi([1, CW(uid)], 1, 1);
                    else
                        NumCollision = NumCollision + 1;
                        % backoff(winner_ix) = randi([1, CW(winner_ix)], 1, 1);
                        CW(uid)=min(CW(uid)*2, CWmax);
%                         backoff(uid) = randi([1, CW(uid)], 1, 1);  
                    end
                    backoff(uid) = randsample(CW(uid),1);
                    % TODO: update the backoff counter of each sender
                    % TODO: modify this based on exponential random backoff
                    backoff(uid) = CWmin/8                           

                    % TODO: may modify this in your final code since CSMA/CA will always have a winner
                    % if (length(winner) > 0)     % flag the medium as busy and update the statistics
                        isBusy = 1;
                        txTime = round(max((StaPktQ(winner)./StaRate(winner)))./SlotTime)
                        NumTx = NumTx + 1;
                    % end

                    StaPktCnt
                    SentBitCnt
                end
            else
                txTime = txTime - 1
                if (txTime <= 0)
                    isBusy = 0;
                    'Channel becomes idle'
                end
            end
        end
        SentBitCnt
        NumTx
        NumCollision
        tput = SentBitCnt/(SimuTime*SlotTime)./1e6
        total_tput(i,j) = sum(tput)
    end    
end
bar(NumSta,mean(total_tput,2))
xlabel('Number of STAs');
ylabel('Average total throughput [Mb/s]');
legend('CSMA/CA');
grid;
saveas(gcf,'rate.jpg','jpg')

function [InterPktTime, StaPktQ, StaPktCnt] = generate_pkt(InterPktTime, nn, PktLen, SlotTime, StaPktQ, StaPktCnt,lambda)
InterPktTime = InterPktTime - 1;
for u=1:length(InterPktTime)
    if(floor(InterPktTime(u)) == 0)
        StaPktCnt(u) = StaPktCnt(u) + 1;
        StaPktQ(u,StaPktCnt(u)) = PktLen;
%         InterPktTime(u)=log(1-rand)/-lambda(u)/SlotTime;  % TODO: update the interarrival time here
        InterPktTime(u)=1/lambda(u)/SlotTime;
        if (u == nn)        % for debugging
            StaPktCnt
        end
    end
end
end
%% return;
