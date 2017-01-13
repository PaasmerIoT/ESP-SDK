#!/bin/bash
echo "Installing...\n";

sudo mkdir -p ~/.aws
sudo chmod 777 ~/.aws
cat > ~/.aws/config << EOF1
[default]
region = us-west-2
EOF1

accesskey=$(echo "U2FsdGVkX1904GIBR/J1JHaexBsYkU151ON7m0qqDvXHZP8OsLxZRH7zETfqopS6tB/bVMUfYJHBzkPQ/67R2g==" | openssl enc -aes-128-cbc -a -d -salt -pass pass:asdfghjkl);

keyid=$(echo "U2FsdGVkX19XbOtwglyiBxjyEME74FjnlS5KrbdvXHQGbUC/BulYsgg+a35BR64W" | openssl enc -aes-128-cbc -a -d -salt -pass pass:asdfghjkl);


echo "[default]
aws_secret_access_key = $accesskey
aws_access_key_id = $keyid
" > ~/.aws/credentials



#echo "[default]
#region = us-west-2" > ~/.aws/config


endpoint=$(sudo aws iot describe-endpoint | grep "endpoint" | awk '{print $2}');
sudo openssl ecparam -out ecckey.key -name prime256v1 -genkey
yes "" | sudo openssl req -new -sha256 -key ecckey.key -nodes -out eccCsr.csr
keys=$(sudo aws iot create-certificate-from-csr --certificate-signing-request file://eccCsr.csr --certificate-pem-outfile eccCert.crt --set-as-active);

ARN=$(echo $keys|tr "," "\n"|grep "certificateArn"|awk '{print $2}');

echo $ARN
keytest=$(echo $keys |tr "," "\n"|grep "certificatePem" | cut -d : -f 2| sed -e 's/\\n/\\r\\n\"\"/g');
keytest=$(echo "${keytest::-2}");
client_key=$(cat ecckey.key | sed -e 's/^/"/g' | sed -e 's/$/\\r\\n"/g')
#echo $client_key

echo "// PAASMER IoT client endpoint
const char *client_endpoint = $endpoint;
// IoT device certificate (ECC)
const char *client_cert = $keytest;
// PAASMER IoT device private key (ECC)
const char *client_key = $client_key;
" > client_config.c

no=$RANDOM
sudo aws iot create-policy --policy-name Paasmer-thing-policy-$no --policy-document '{ "Version": "2012-10-17", "Statement": [{"Action": ["iot:*"], "Resource": ["*"], "Effect": "Allow" }] }'

function Paasmer {
  echo "alias PAASMER='sudo aws iot attach-principal-policy --policy-name Paasmer-thing-policy-$no --principal $ARN'" >> ~/.bashrc
  #export $(theja)
}

#alias Paasmer='sudo aws iot attach-principal-policy --policy-name test-thing-policy-'$no' --principal '$ARN''
Paasmer

xterm -e "source ~/.bashrc"
xterm -e "sudo PAASMER"

echo '#!/bin/bash
#xterm -e source ~/.bashrc;
xterm -e sudo PAASMER;
echo "Done Installing...";' > Configure.sh

sudo chmod 777 Configure.sh
bash Configure.sh

cd ../../
#sudo sed -i 's/alias PAASMER/#alias PAASMER/g' ~/.bashrc

#echo "*******************************************************************"
#echo "*      Please open new Terminal and type bellow command           *"
#echo "*      $  Configure.sh                                                 *"
#echo "*******************************************************************"


#out=$(alias Paasmer='sudo aws iot attach-principal-policy --policy-name test-thing-policy-'$no' --principal '$ARN''; Paasmer);

#sudo aws configure

#keys=$(sudo aws iot create-certificate-from-csr --certificate-signing-request file://eccCsr.csr --certificate-pem-outfile eccCert.crt --set-as-active|grep "certificatePem" | cut -d : -f 2| sed -e 's/\\n/\\r\\n\"\"/g');

#keytest=$(echo $keys | sed -e 's/\\n/\\r\\n\"\"/g');

#sudo sed -i 's/\"<your-prefix>.iot.<aws-region>.amazonaws.com\"/'$endpoint'/g' client_config.c

#comd=$(echo "sudo aws iot attach-principal-policy --policy-name test-thing-policy-$no --principal $ARN");

#cmd=$(sudo aws iot attach-principal-policy --policy-name test-thing-policy-$no --principal $ARN);

#mosquitto_pub --cafile AWSIoTCACert.crt --cert eccCert.crt --key ecckey.key -h a3rwl3kghmkdtx.iot.us-west-2.amazonaws.com -d -p 8883 -q 1 -t foo/bar -i test --tls-version tlsv1.2 -m “HelloWorld” --ciphers ECDHE-ECDSA-AES128-GCM-SHA256
