%% BPSK transmission over AWGN channel
close all;clear all;clc;           % BPSK
dist=100:100:400;        % distance in meters
PtdBm=10;                % transmit power in dBm
PndBm=-85;              % noise power in dBm
Pt=10^(PtdBm/10)/1000;  % transmit power in watt
Pn=10^(PndBm/10)/1000;  % noise power in watt
Bit_Length=1e3;         % number of bits transmitted

%% Friss Path Loss Model
Gt=1;
Gr=1;
freq=2.4e9;


% TODO: Calculate Pr(d)
Pr=ones(length(dist),1);    % TODO: replace this with Friis' model
for d=1:length(dist)
    Pr(d,1) = Pt*Gr*Gt*(((3e8/freq)/(4*pi*dist(1,d)))^2);
end
%TODO END

%% BPSK Transmission over AWGN channel
tx_data = randi(2, 1, Bit_Length) - 1;                  % random between 0 and 1
%% TODO-2
%% BPSK: {1,0} -> {1+0i, -1+0i}
%% QPSK: {11,10,01,00} -> {1+i, -1+i, -1-i, 1-i} * scaling factor
%% 16QAM: {1111, 1110, 1101, 1100, 1011, 1010, 1001, 1000, 0111, 0110, 0101, 0100, 0011, 0010, 0001, 0000}
%% -> {3a+3ai, 3a+ai, a+3ai, a+ai, -a+3ai, -3a+3ai, -3a+ai, -a+ai, 3a-ai, 3a-3ai, a-ai, a-3i, -a-ai, -a-3ai, -3a-ai, -3a-3ai}
n=(randn(1,Bit_Length)+randn(1,Bit_Length)*i)/sqrt(2);  % AWGN noises
n=n*sqrt(Pn);
a=1/sqrt(10);
for mod_order=[1,2,4]
    if(mod_order==1)
        x(mod_order,:)=(tx_data.*2-1)+0i;                                  % TODO-2: change it to three different modulated symbols
    end
    if(mod_order==2)
        for l=1:2:length(tx_data)
            x(mod_order,(l+1)/2)=(abs(tx_data(l)-tx_data(l+1))*(-2)+1)+(tx_data(l)*2-1)*1i;
            x(mod_order,(l+1)/2)=x(mod_order,(l+1)/2)/sqrt(2);
        end
    end
    if(mod_order==4)
        for l=1:4:length(tx_data)
            if(tx_data(l)==1 && tx_data(l+1)==1)
                x(mod_order, (l+3)/4)=(tx_data(l+2)*2+1)*a+(tx_data(l+3)*2+1)*a*1i;
            end
            if(tx_data(l)==1 && tx_data(l+1)==0)
                x(mod_order, (l+3)/4)=(abs(tx_data(l+2)-tx_data(l+3))*2+1)*(-a)+(tx_data(l+2)*2+1)*a*1i;
            end
            if(tx_data(l)==0 && tx_data(l+1)==1)
                x(mod_order, (l+3)/4)=(tx_data(l+2)*2+1)*a+((tx_data(l+3)-1)*(-2)+1)*(a*(-1i));
            end
            if(tx_data(l)==0 && tx_data(l+1)==0)
                x(mod_order, (l+3)/4)=((tx_data(l+2)-1)*(-2)+1)*(-a)+((tx_data(l+3)-1)*(-2)+1)*(a*(-1i));
            end
        end
    end   
    %% TODO END
    for d=1:length(dist)
        y(mod_order,d,:)=sqrt(Pr(d))*x(mod_order,:)+n;
    end
end

%% Equalization
% Detection Scheme:(Soft Detection)
% +1 if o/p >=0
% -1 if o/p<0
% Error if input and output are of different signs


for mod_order=[1,2,4]
    figure('units','normalized','outerposition',[0 0 1 1])
	sgtitle(sprintf('Modulation order: %d', mod_order)); 
    for d=1:length(dist)
        % TODO: s = y/Pr
        % TODO: x_est = 1 if real(s) >= 0; otherwise, x_est = -1
        if(mod_order==1)
            s(mod_order,:)=y(mod_order,d,:)/sqrt(Pr(d));
            for i=1:Bit_Length
                if real(s(mod_order,i))>=0
                    x_est(i)=1;
                else 
                    x_est(i)=0;
                end    
            end
        end
        if(mod_order==2)
            for i=1:Bit_Length/2
                s(mod_order,i)=y(mod_order,d,i)/sqrt(Pr(d));
            end
            for i=1:Bit_Length/2
                if real(s(mod_order,i))>=0
                    if imag(s(mod_order,i))>=0
                        x_est(i*2-1:i*2)=[1,1];
                    else
                        x_est(i*2-1:i*2)=[0,0];
                    end
                else   
                    if imag(s(mod_order,i))>=0
                        x_est(i*2-1:i*2)=[1,0];
                    else
                        x_est(i*2-1:i*2)=[0,1];
                    end
                end
            end
        end
        if(mod_order==4)
            for i=1:Bit_Length/4
                s(mod_order,i)=y(mod_order,d,i)/sqrt(Pr(d));
            end
            for i=1:Bit_Length/4
                if real(s(mod_order,i))>=2*a
                        if imag(s(mod_order,i))>=2*a
                            x_est(i*4-3:i*4)=[1,1,1,1];
                        elseif imag(s(mod_order,i))>=0 
                            x_est(i*4-3:i*4)=[1,1,1,0]; 
                        elseif imag(s(mod_order,i))<=-2*a 
                            x_est(i*4-3:i*4)=[0,1,1,0];
                        elseif imag(s(mod_order,i))<=0 
                            x_est(i*4-3:i*4)=[0,1,1,1];  
                        end
                elseif real(s(mod_order,i))>=0
                        if imag(s(mod_order,i))>=2*a
                            x_est(i*4-3:i*4)=[1,1,0,1];
                        elseif imag(s(mod_order,i))>=0 
                            x_est(i*4-3:i*4)=[1,1,0,0]; 
                        elseif imag(s(mod_order,i))<=-2*a 
                            x_est(i*4-3:i*4)=[0,1,0,0];
                        elseif imag(s(mod_order,i))<=0 
                            x_est(i*4-3:i*4)=[0,1,0,1];  
                        end

                elseif real(s(mod_order,i)) <=-2*a
                        if imag(s(mod_order,i))>=2*a
                            x_est(i*4-3:i*4)=[1,0,1,0];
                        elseif imag(s(mod_order,i))>=0 
                            x_est(i*4-3:i*4)=[1,0,0,1]; 
                        elseif imag(s(mod_order,i))<=-2*a 
                            x_est(i*4-3:i*4)=[0,0,0,0];
                        elseif imag(s(mod_order,i))<=0 
                            x_est(i*4-3:i*4)=[0,0,0,1];  
                        end

                elseif real(s(mod_order,i)) <=0
                        if imag(s(mod_order,i))>=2*a
                            x_est(i*4-3:i*4)=[1,0,1,1];
                        elseif imag(s(mod_order,i))>=0 
                            x_est(i*4-3:i*4)=[1,0,0,0]; 
                        elseif imag(s(mod_order,i))<=-2*a 
                            x_est(i*4-3:i*4)=[0,0,1,0];
                        elseif imag(s(mod_order,i))<=0 
                            x_est(i*4-3:i*4)=[0,0,1,1];  
                        end
                    end        
            end
        end
        % TODO END

        SNR(d,mod_order)=Pr(d)/Pn;
        SNRdB(d,mod_order)=10*log10(SNR(d,mod_order));
        % TODO-2: demodulate x_est to x' for various modulation schemes and calculate BER_simulated(d)
        % TODO: noise = s - x, and, then, calculate SNR_simulated(d)
        sum=0
        for j=1:Bit_Length
            if(x_est(j)==tx_data(j))
                sum=sum+1;
            end
        end
        BER_simulated(d,mod_order)=(Bit_Length-sum)/Bit_Length;
        for j=1:Bit_Length
            noise(d,j)=s(mod_order,j)-x(mod_order,j);
        end
        sn=0;
        rn=0;
        for i=1:length(noise)
            sn=sn+real(noise(d,i))^2+imag(noise(d,i))^2;
            rn=rn+real(x(mod_order,i))^2+imag(x(mod_order,i))^2;
        end
        SNRdB_simulated(d,mod_order)=rn/sn;
        %TODO END
        subplot(2, 2, d)
        hold on;
        plot(s(mod_order,:),'bx');       % TODO: replace y with s
        plot(x(mod_order,:),'ro');
        hold off;
        xlim([-2,2]);
        ylim([-2,2]);
        title(sprintf('Constellation points d=%d', dist(d)));
        legend('decoded samples', 'transmitted samples');
        grid
    end
    filename = sprintf('IQ_%d.jpg', mod_order);
    saveas(gcf,filename,'jpg')
end

%% TODO-2: modify the figures to compare three modulation schemes
figure('units','normalized','outerposition',[0 0 1 1])
hold on;
semilogy(dist,SNRdB_simulated(:,1),'bo-','linewidth',2.0);
semilogy(dist,SNRdB_simulated(:,2),'rv--','linewidth',2.0);
semilogy(dist,SNRdB_simulated(:,4),'mx-.','linewidth',2.0);
hold off;
title('SNR');
xlabel('Distance [m]');
ylabel('SNR [dB]');
legend('BPSK','QPSK','16QAM');
axis tight 
grid
saveas(gcf,'SNR.jpg','jpg')

figure('units','normalized','outerposition',[0 0 1 1])
hold on;
semilogy(dist,BER_simulated(:,1),'bo-','linewidth',2.0);
semilogy(dist,BER_simulated(:,2),'rv--','linewidth',2.0);
semilogy(dist,BER_simulated(:,4),'mx-.','linewidth',2.0);
hold off;
title('BER');
xlabel('Distance [m]');
ylabel('BER');
legend('BPSK','QPSK','16QAM');
axis tight 
grid
saveas(gcf,'BER.jpg','jpg')
return;
