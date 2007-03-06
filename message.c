/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * See LICENSE file for license details.
 */
#include "ixp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IXP_QIDSZ (sizeof(uchar) + sizeof(uint)\
		+ sizeof(uvlong))

static ushort
sizeof_string(const char *s) {
	return sizeof(ushort) + strlen(s);
}

ushort
ixp_sizeof_stat(Stat * stat) {
	return IXP_QIDSZ
		+ 2 * sizeof(ushort)
		+ 4 * sizeof(uint)
		+ sizeof(uvlong)
		+ sizeof_string(stat->name)
		+ sizeof_string(stat->uid)
		+ sizeof_string(stat->gid)
		+ sizeof_string(stat->muid);
}

uint
ixp_fcall2msg(void *msg, Fcall *fcall, uint msglen) {
	uint i = sizeof(uchar) +
		sizeof(ushort) + sizeof(uint);
	int msize = msglen - i;
	uchar *p = (uchar*)msg + i;

	switch (fcall->type) {
	case TVERSION:
	case RVERSION:
		ixp_pack_u32(&p, &msize, fcall->msize);
		ixp_pack_string(&p, &msize, fcall->version);
		break;
	case TAUTH:
		ixp_pack_u32(&p, &msize, fcall->afid);
		ixp_pack_string(&p, &msize, fcall->uname);
		ixp_pack_string(&p, &msize, fcall->aname);
		break;
	case RAUTH:
		ixp_pack_qid(&p, &msize, &fcall->aqid);
		break;
	case RATTACH:
		ixp_pack_qid(&p, &msize, &fcall->qid);
		break;
	case TATTACH:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u32(&p, &msize, fcall->afid);
		ixp_pack_string(&p, &msize, fcall->uname);
		ixp_pack_string(&p, &msize, fcall->aname);
		break;
	case RERROR:
		ixp_pack_string(&p, &msize, fcall->ename);
		break;
	case TFLUSH:
		ixp_pack_u16(&p, &msize, fcall->oldtag);
		break;
	case TWALK:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u32(&p, &msize, fcall->newfid);
		ixp_pack_u16(&p, &msize, fcall->nwname);
		for(i = 0; i < fcall->nwname; i++)
			ixp_pack_string(&p, &msize, fcall->wname[i]);
		break;
	case RWALK:
		ixp_pack_u16(&p, &msize, fcall->nwqid);
		for(i = 0; i < fcall->nwqid; i++)
			ixp_pack_qid(&p, &msize, &fcall->wqid[i]);
		break;
	case TOPEN:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u8(&p, &msize, fcall->mode);
		break;
	case ROPEN:
	case RCREATE:
		ixp_pack_qid(&p, &msize, &fcall->qid);
		ixp_pack_u32(&p, &msize, fcall->iounit);
		break;
	case TCREATE:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_string(&p, &msize, fcall->name);
		ixp_pack_u32(&p, &msize, fcall->perm);
		ixp_pack_u8(&p, &msize, fcall->mode);
		break;
	case TREAD:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u64(&p, &msize, fcall->offset);
		ixp_pack_u32(&p, &msize, fcall->count);
		break;
	case RREAD:
		ixp_pack_u32(&p, &msize, fcall->count);
		ixp_pack_data(&p, &msize, (uchar *)fcall->data, fcall->count);
		break;
	case TWRITE:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u64(&p, &msize, fcall->offset);
		ixp_pack_u32(&p, &msize, fcall->count);
		ixp_pack_data(&p, &msize, (uchar *)fcall->data, fcall->count);
		break;
	case RWRITE:
		ixp_pack_u32(&p, &msize, fcall->count);
		break;
	case TCLUNK:
	case TREMOVE:
	case TSTAT:
		ixp_pack_u32(&p, &msize, fcall->fid);
		break;
	case RSTAT:
		ixp_pack_u16(&p, &msize, fcall->nstat);
		ixp_pack_data(&p, &msize, fcall->stat, fcall->nstat);
		break;
	case TWSTAT:
		ixp_pack_u32(&p, &msize, fcall->fid);
		ixp_pack_u16(&p, &msize, fcall->nstat);
		ixp_pack_data(&p, &msize, fcall->stat, fcall->nstat);
		break;
	}
	if(msize < 0)
		return 0;
	msize = msglen - msize;
	ixp_pack_prefix(msg, msize, fcall->type, fcall->tag);
	return msize;
}

uint
ixp_msg2fcall(Fcall *fcall, void *msg, uint msglen) {
	int msize;
	uint i, tsize;
	ushort len;
	uchar *p = msg;
	ixp_unpack_prefix(&p, (uint *)&msize, &fcall->type, &fcall->tag);
	tsize = msize;

	if(msize > msglen)          /* bad message */
		return 0;
	switch (fcall->type) {
	case TVERSION:
	case RVERSION:
		ixp_unpack_u32(&p, &msize, &fcall->msize);
		ixp_unpack_string(&p, &msize, &fcall->version, &len);
		break;
	case TAUTH:
		ixp_unpack_u32(&p, &msize, &fcall->afid);
		ixp_unpack_string(&p, &msize, &fcall->uname, &len);
		ixp_unpack_string(&p, &msize, &fcall->aname, &len);
		break;
	case RAUTH:
		ixp_unpack_qid(&p, &msize, &fcall->aqid);
		break;
	case RATTACH:
		ixp_unpack_qid(&p, &msize, &fcall->qid);
		break;
	case TATTACH:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u32(&p, &msize, &fcall->afid);
		ixp_unpack_string(&p, &msize, &fcall->uname, &len);
		ixp_unpack_string(&p, &msize, &fcall->aname, &len);
		break;
	case RERROR:
		ixp_unpack_string(&p, &msize, &fcall->ename, &len);
		break;
	case TFLUSH:
		ixp_unpack_u16(&p, &msize, &fcall->oldtag);
		break;
	case TWALK:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u32(&p, &msize, &fcall->newfid);
		ixp_unpack_u16(&p, &msize, &fcall->nwname);
		ixp_unpack_strings(&p, &msize, fcall->nwname, fcall->wname);
		break;
	case RWALK:
		ixp_unpack_u16(&p, &msize, &fcall->nwqid);
		for(i = 0; i < fcall->nwqid; i++)
			ixp_unpack_qid(&p, &msize, &fcall->wqid[i]);
		break;
	case TOPEN:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u8(&p, &msize, &fcall->mode);
		break;
	case ROPEN:
	case RCREATE:
		ixp_unpack_qid(&p, &msize, &fcall->qid);
		ixp_unpack_u32(&p, &msize, &fcall->iounit);
		break;
	case TCREATE:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_string(&p, &msize, &fcall->name, &len);
		ixp_unpack_u32(&p, &msize, &fcall->perm);
		ixp_unpack_u8(&p, &msize, &fcall->mode);
		break;
	case TREAD:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u64(&p, &msize, &fcall->offset);
		ixp_unpack_u32(&p, &msize, &fcall->count);
		break;
	case RREAD:
		ixp_unpack_u32(&p, &msize, &fcall->count);
		ixp_unpack_data(&p, &msize, (void *)&fcall->data, fcall->count);
		break;
	case TWRITE:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u64(&p, &msize, &fcall->offset);
		ixp_unpack_u32(&p, &msize, &fcall->count);
		ixp_unpack_data(&p, &msize, (void *)&fcall->data, fcall->count);
		break;
	case RWRITE:
		ixp_unpack_u32(&p, &msize, &fcall->count);
		break;
	case TCLUNK:
	case TREMOVE:
	case TSTAT:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		break;
	case RSTAT:
		ixp_unpack_u16(&p, &msize, &len);
		ixp_unpack_data(&p, &msize, &fcall->stat, len);
		break;
	case TWSTAT:
		ixp_unpack_u32(&p, &msize, &fcall->fid);
		ixp_unpack_u16(&p, &msize, &len);
		ixp_unpack_data(&p, &msize, &fcall->stat, len);
		break;
	}
	if(msize > 0)
		return tsize;
	return 0;
}
